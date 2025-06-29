#pragma once

// init
#define IS_DEBUG (1)
// #define IS_DEBUG (0)

#define ENABLE_HTTPS (1) // Use HTTPS by default, can be changed to HTTP by defining HTTP
// #define HTTPS (0) // Use HTTPS by default, can be changed to HTTP by defining HTTP

#define ENABLE_PRINT_LISTEN_INFO (1) // Print server address and port when starting the server
// #define ENABLE_PRINT_LISTEN_INFO (0) // Do not print server address and port when

#define NAME "cppNetworkUtil"
#define PROTOCOL "HTTP/1.1"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#ifdef ENABLE_HTTPS
#define DEFAULT_SERVER_PORT 443 // Default port for HTTPS
#elif
#define DEFAULT_SERVER_PORT 80
#endif

#define PUBLIC_KET_PATH "server.crt"  // Public key path
#define PRIVATE_KEY_PATH "server.key" // Private key path

#define INFINITY 2147483647
#define BUFFERSIZE 4096

// 错误处理宏，用于打印 OpenSSL 错误并退出
#define HANDLE_ERROR(msg)                           \
    do                                              \
    {                                               \
        ERR_print_errors_fp(stderr);                \
        std::cerr << "Error: " << msg << std::endl; \
        exit(EXIT_FAILURE);                         \
    } while (0)

// C++ standard library headers

#include <iostream>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlobj.h>

#pragma comment(lib, "ws2_32.lib")

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>

typedef int SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define WSAGetLastError() errno

#endif

#include <map>
#include <ctime>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <fstream> // For std::ifstream
#include <queue>
#include <condition_variable>
#include <functional> // For std::function
#include <future>     // For std::future, std::packaged_task
#include <algorithm>  // For std::min

// openssl
#include "openssl/ssl.h"
#include "openssl/err.h"

// ThreadPool 类定义
class ThreadPool
{
public:
    // 构造函数：初始化线程池，创建指定数量的工作线程
    explicit ThreadPool(size_t numThreads);

    // 提交任务到线程池
    // 任务可以是任何可调用对象（函数、lambda、函数指针等）
    // 返回一个 std::future，用于获取任务的返回值
    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;

    // 析构函数：在线程池对象销毁时，停止所有工作线程并等待它们完成
    ~ThreadPool();

private:
    // 禁止拷贝构造和拷贝赋值，因为线程池管理资源（线程）不适合拷贝
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    std::vector<std::thread> workers;        // 存储工作线程的容器
    std::queue<std::function<void()>> tasks; // 存储待执行任务的队列

    std::mutex queueMutex;             // 用于保护任务队列的互斥量
    std::condition_variable condition; // 用于线程间通信的条件变量
    bool stop;                         // 线程池停止标志
};

class cppNetworkUtil
{
public:
    int port = DEFAULT_SERVER_PORT; // 服务器端口

    struct responseHeaderParameters
    {
        int status;                   // status code
        std::string mime_type;        // mime type
        std::string content_language; // content language
        std::string cookie;           // cookie

        responseHeaderParameters(int in_status = 200, std::string in_mime_type = "*/*", std::string in_content_language = "en-us", std::string in_cookie = "") : status(in_status), mime_type(in_mime_type), content_language(in_content_language), cookie(in_cookie) {}
    } response_header_parameters;

    struct requestHeaderParameters
    {
        std::string method;     // request method
        std::string connection; // connection type
        std::string host;       // host
        int port;               // port

        requestHeaderParameters(std::string in_method = "GET", std::string in_connection = "close", std::string in_host = "example.com", int in_port = 80) : method(in_method), connection(in_connection), host(in_host), port(in_port) {}
    } request_header_parameters;

    // 用于存储解析过的post的multipart数据
    struct multipartData
    {
        std::string name;
        std::string data;
        std::string filename;
        std::string content_type;

        multipartData() : name(""), data(""), filename(""), content_type("") {}

        void print() const
        {
            std::cout << "Name: " << name << "\n";
            if (!filename.empty())
            {
                std::cout << "  Filename: " << filename << "\n";
            }
            if (!content_type.empty())
            {
                std::cout << "  Content-Type: " << content_type << "\n";
            }
            std::cout << "  Data: " << data << "\n";
            std::cout << "\n";
            //		std::std::cout << "name:" << name << "  Data (partial): " << data.substr(0, std::min((size_t)50, data.length())) << (data.length() > 50 ? "..." : "") << "\n" << "filename" << filename << "content_type=" << content_type << "\n";
        }
    };

    /**
     * @brief Get the method in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Invalid request line: No space found after method
     *
     * @return method
     */
    static std::string getHeaderMethod(const std::string buffer);

    /**
     * @brief Get the url in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Invalid request line: No space found after method
     * @throws Invalid request line: No space found after url
     *
     * @return url
     */
    static std::string getGetHeaderUrl(const std::string buffer);

    /**
     * @brief Get the content size in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     * @throws Parse Content-Length failed
     * @throws Find body failed
     *
     * @return content size
     */
    static int getPostContentSize(const std::string buffer);

    /**
     * @brief Get the Content-Type in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     *
     * @return Content-Type
     */
    static std::string getPostContentType(const std::string buffer);

    /**
     * @brief Get the Boundary in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     *
     * @return Boundary
     */
    static std::string getPostContentBoundary(const std::string buffer);

    /**
     * @brief Get the content body in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     *
     * @return content body
     */
    static std::string getPostContentBody(const std::string buffer);

    /**
     * @brief Make a request header
     *
     * @param parameters (headerParameters) parameter
     *
     * @return header
     */
    static std::string buildResponseHeader(responseHeaderParameters parameters);

    /**
     * @brief Make a request header
     *
     * @param parameter (requestHeaderParameters) parameter
     *
     * @return header
     */
    static std::string buildRequestHeader(requestHeaderParameters parameter);

    /**
     * @brief Decode a URL-encoded string
     *
     * @param encodedString (const std::string &) The string to be decoded
     *
     * @return Decoded string
     */
    static std::string urlDecode(const std::string &encodedString);

    /**
     * @brief Parse URL parameters from a RESTful API style URL
     *
     * @param url (std::string) The URL to parse
     *
     * @return A vector of strings representing the parameters in the URL
     */
    static std::vector<std::string> getURLParameterRestfulapi(std::string url);

    /**
     * @brief Parse URL parameters from URL query string
     *
     * @param url (std::string) The URL to parse
     *
     * @throws Invalid URL format
     *
     * @return A map of key-value pairs representing the query parameters
     */
    static std::map<std::string, std::string> parseUrlQueryParameters(const std::string &url);

    /**
     * @brief Parse Multipart data from a POST request body
     *
     * @param boundary (const std::string &boundary) The boundary string used to separate parts in the multipart data
     * @param body (const std::string &body) The body of the POST request containing multipart data
     *
     * @return A vector of multipartData objects, each representing a part of the multipart data
     */
    static std::vector<multipartData> parseMultipart(const std::string &boundary, const std::string &body);

    /**
     * @brief Send data to the socket using HTTP protocol
     *
     * @param data (const std::string) Data to be sent
     * @param socket (SOCKET) socket to send data to
     */
    static void sendDataToHttpSocket(const std::string data, SOCKET socket);

    /**
     * @brief Send data to the socket using HTTPS protocol
     *
     * @param data (const std::string) Data to be sent
     * @param ssl (SSL *) SSL structure for sending data
     */
    static void sendDataToHttpsSocket(const std::string data, SSL *ssl);

    /**
     * @brief Send data to the host
     *
     * @param host (const std::string &) Host address
     * @param port (int) Port number
     * @param request (const std::string &) Request data to be sent
     * @param header (std::string &) Header to be filled with the response header
     * @param content (std::string &) Content to be filled with the response content
     *
     * @throws WSAStartup failed
     * @throws getaddrinfo failed
     * @throws Unable to connect to server
     * @throws Send failed
     * @throws shutdown failed
     * @throws Invalid HTTP response format
     */
    static void sendDataToHost(const std::string &host, int port, const std::string &request, std::string &header, std::string &content);

    /**
     * @brief start server
     *
     * @param port (int) bind port
     * @param func (std::function<void(const std::string, SOCKET, SSL *)>) Function to process the received data
     *
     * @throws WSAStartup failed
     * @throws Create socket failed
     * @throws Setsockopt failed
     * @throws Bind failed
     * @throws Listen failed
     *
     * @return server_socket
     */
    void run(std::function<void(const std::string, SOCKET, SSL *)> func);

    /**
     * @brief Automatically executed when leaving scope
     */
    ~cppNetworkUtil();

private:
    SSL_CTX *ctx = nullptr;

    void process(SOCKET client_socket, std::function<void(const std::string, SOCKET, SSL *)> func);
};