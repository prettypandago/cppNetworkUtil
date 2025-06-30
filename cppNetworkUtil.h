#pragma once

// include
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
#include <sstream>
#include <fstream> // For std::ifstream
#include <condition_variable>
#include <algorithm>  // For std::min
#include <functional> // For std::function
#include <future>     // For std::future, std::packaged_task
#include <queue>
#include <mutex>
#include <cstdio>
#include <ctime>

// openssl
#include "openssl/ssl.h"
#include "openssl/err.h"

// color
#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define GRAY "\033[1;30m"
#define LIGHT_GRAY "\033[0;37m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define WHITE "\033[1;37m"
#define WHITE_BG_WHITE "\033[37;47m"
#define BLACK_BG_BLACK "\033[30;40m"

#define BG_RED "\033[0;42;41m"
#define BG_LIGHT_RED "\033[1;41m"
#define BG_GREEN "\033[0;42;42m"
#define BG_LIGHT_GREEN "\033[1;42m"
#define BG_BLUE "\033[0;42;44m"
#define BG_LIGHT_BLUE "\033[1;44m"
#define BG_GRAY "\033[1;40m"
#define BG_LIGHT_GRAY "\033[0;47m"
#define BG_CYAN "\033[0;46m"
#define BG_LIGHT_CYAN "\033[1;46m"
#define BG_PURPLE "\033[0;45m"
#define BG_LIGHT_PURPLE "\033[1;45m"
#define BG_BROWN "\033[0;43m"
#define BG_YELLOW "\033[1;43m"
#define BG_WHITE "\033[1;47m"

// init
#define NAME "cppNetworkUtil"
#define PROTOCOL "HTTP/1.1"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#ifdef DISABLE_HTTPS
#define DEFAULT_SERVER_PORT 80 // Default port for HTTP
#else
#define DEFAULT_SERVER_PORT 443 // Default port for HTTPS
#endif

#define PUBLIC_KET_PATH "server.crt"  // Public key path
#define PRIVATE_KEY_PATH "server.key" // Private key path

#define INFINITY 2147483647
#define BUFFERSIZE 4096

// log
#ifdef IS_DEBUG
#define log_e(fmt, ...)                                                                                             \
    do                                                                                                              \
    {                                                                                                               \
        time_t t = time(nullptr);                                                                                   \
        struct tm tm_info;                                                                                          \
        localtime_s(&tm_info, &t);                                                                                  \
        char time_buf[32];                                                                                          \
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                        \
        printf(RED "[ERROR][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define log_e(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif

#ifdef IS_DEBUG
#define log_w(fmt, ...)                                                                                                      \
    do                                                                                                                       \
    {                                                                                                                        \
        if (IS_DEBUG)                                                                                                        \
        {                                                                                                                    \
            time_t t = time(nullptr);                                                                                        \
            struct tm tm_info;                                                                                               \
            localtime_s(&tm_info, &t);                                                                                       \
            char time_buf[32];                                                                                               \
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                             \
            printf(YELLOW "[WARNING][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                                                    \
    } while (0)
#else
#define log_w(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif

#ifdef IS_DEBUG
#define log_d(fmt, ...)                                                                                                  \
    do                                                                                                                   \
    {                                                                                                                    \
        if (IS_DEBUG)                                                                                                    \
        {                                                                                                                \
            time_t t = time(nullptr);                                                                                    \
            struct tm tm_info;                                                                                           \
            localtime_s(&tm_info, &t);                                                                                   \
            char time_buf[32];                                                                                           \
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                         \
            printf(GRAY "[DEBUG][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                                                \
    } while (0)
#else
#define log_d(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif

#define log_i(fmt, ...)                                                                                              \
    do                                                                                                               \
    {                                                                                                                \
        time_t t = time(nullptr);                                                                                    \
        struct tm tm_info;                                                                                           \
        localtime_s(&tm_info, &t);                                                                                   \
        char time_buf[32];                                                                                           \
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                         \
        printf(WHITE "[INFO][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

// 错误处理宏，用于打印 OpenSSL 错误并退出
#ifdef IS_DEBUG
#define HANDLE_ERROR(msg)            \
    do                               \
    {                                \
        printf(RED);                 \
        ERR_print_errors_fp(stderr); \
        printf("Error: %s\n", msg);  \
        printf(NONE);                \
    } while (0)
#else
#define HANDLE_ERROR(msg) \
    do                    \
    {                     \
    } while (0)
#endif

extern SSL_CTX *server_ctx;
extern SSL_CTX *client_ctx;

// threadPool
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

// cppNetworkUtil
class cppNetworkUtil
{
public:
    int port = DEFAULT_SERVER_PORT; // 服务器端口

    struct responseHeaderParameters
    {
        int status;                                              // status code
        std::string mime_type;                                   // mime type
        std::string content_language;                            // content language
        std::string cookie;                                      // cookie
        int Strict_Transport_Security_max_age;                   // Strict-Transport-Security header max-age
        std::string Strict_Transport_Security_includeSubDomains; // Strict-Transport-Security header
        std::string Cache_Control;                               // Cache-Control header

        responseHeaderParameters(int in_status = 200, std::string in_mime_type = "*/*", std::string in_content_language = "en-us", std::string in_cookie = "", int in_Strict_Transport_Security_max_age = 31536000, std::string in_Strict_Transport_Security_includeSubDomains = "preload", std::string in_Cache_Control = "no-cache") : status(in_status), mime_type(in_mime_type), content_language(in_content_language), cookie(in_cookie), Strict_Transport_Security_max_age(in_Strict_Transport_Security_max_age), Strict_Transport_Security_includeSubDomains(in_Strict_Transport_Security_includeSubDomains), Cache_Control(in_Cache_Control) {}
    } response_header_parameters;

    struct requestHeaderParameters
    {
        std::string method;     // request method
        std::string connection; // connection type
        std::string host;       // host
        std::string path;       // path
        int port;               // port

        requestHeaderParameters(std::string in_method = "GET", std::string in_connection = "close", std::string in_host = "example.com", std::string in_path = "/", int in_port = 80) : method(in_method), connection(in_connection), host(in_host), path(in_path), port(in_port) {}
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
     * @brief Get the content size in the http request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     * @throws Parse Content-Length failed
     * @throws Find body failed
     *
     * @return content size
     */
    static int getContentSize(const std::string buffer);

    /**
     * @brief Get the value of a specific header field in the HTTP request header
     *
     * @param headers (const std::string &) The HTTP request header string
     * @param key (const std::string &) The key of the header field to retrieve
     *
     * @return The value of the specified header field, or an empty string if not found
     */
    static std::string getHeaderValue(const std::string &headers, const std::string &key);

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
     * @brief Send data to the host using HTTP protocol
     *
     * @param host (const std::string &) Host address
     * @param path (const std::string &) Path to the resource
     * @param port (int) Port number
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
    static void sendDataToHttpHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content);

    /**
     * @brief Send data to the host using HTTPS protocol
     *
     * @param host (const std::string &) Host address
     * @param path (const std::string &) Path to the resource
     * @param port (int) Port number
     * @param header (std::string &) Header to be filled with the response header
     * @param content (std::string &) Content to be filled with the response content
     * @param enable_CA (bool) Whether to enable CA verification
     *
     * @throws WSAStartup failed
     * @throws Failed to load default CA certificates
     * @throws Failed to create SSL object
     * @throws Failed to create BIO connection
     * @throws Failed to connect to server
     * @throws SSL handshake failed
     * @throws SSL read failed
     * @throws Invalid HTTP response format
     */
    static void sendDataToHttpsHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA = true);

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
    void process(SOCKET client_socket, std::function<void(const std::string, SOCKET, SSL *)> func);
};