#pragma once

// 前向声明实现类
class cppNetworkUtilPimpl;
class serverCallback;

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
#include <memory> //For std::unique_ptr

#include "defines.h"
#include "log.h"

// cppNetworkUtil
class cppNetworkUtil
{
public:
    int port = DEFAULT_SERVER_PORT; // 服务器端口

    /**
     * @brief Get the client connections IP address
     *
     * @param client_socket (SOCKET) Client socket
     *
     * @return Client IP address
     */
    std::string get_client_connections_ip(SOCKET client_socket);

    /**
     * @brief Get the client connections port
     *
     * @param client_socket (SOCKET) Client socket
     *
     * @return Client port
     */
    int get_client_connections_port(SOCKET client_socket);

    /**
     * @brief Get the client connections recv buffer
     *
     * @param client_socket (SOCKET) Client socket
     *
     * @return Client recv buffer
     */
    std::string get_client_connections_recv_buffer(SOCKET client_socket);

    /**
     * @brief Get the method in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Invalid request line: No space found after method
     *
     * @return method
     */
    std::string getHeaderMethod(const std::string buffer);

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
    std::string getGetHeaderUrl(const std::string buffer);

    /**
     * @brief Get the content size in the http request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     * @throws Parse Content-Length failed
     * @throws Parse Content-Length failed with unknown error
     * @throws Find body failed
     *
     * @return content size
     */
    int getContentSize(const std::string buffer);

    /**
     * @brief Get the value of a specific header field in the HTTP request header
     *
     * @param headers (const std::string &) The HTTP request header string
     * @param key (const std::string &) The key of the header field to retrieve
     *
     * @return The value of the specified header field, or an empty string if not found
     */
    std::string getHeaderValue(const std::string &headers, const std::string &key);

    /**
     * @brief Get the content body in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Find Content-Length failed
     * @throws Parse Content-Length failed
     * @throws Parse Content-Length failed with unknown error
     * @throws Find body failed
     * @throws Size does not meet the requirements
     *
     * @return content body
     */
    std::string getPostContentBody(const std::string buffer);

    /**
     * @brief Make a request header
     *
     * @param parameters (headerParameters) parameter
     *
     * @return header
     */
    std::string buildResponseHeader(responseHeaderParameters parameters);

    /**
     * @brief Make a request header
     *
     * @param parameter (requestHeaderParameters) parameter
     *
     * @return header
     */
    std::string buildRequestHeader(requestHeaderParameters parameter);

    /**
     * @brief Decode a URL-encoded string
     *
     * @param encodedString (const std::string &) The string to be decoded
     *
     * @return Decoded string
     */
    std::string urlDecode(const std::string &encodedString);

    /**
     * @brief Parse URL parameters from a RESTful API style URL
     *
     * @param url (std::string) The URL to parse
     *
     * @return A vector of strings representing the parameters in the URL
     */
    std::vector<std::string> getURLParameterRestfulapi(std::string url);

    /**
     * @brief Parse URL parameters from URL query string
     *
     * @param url (std::string) The URL to parse
     *
     * @throws Invalid URL format
     *
     * @return A map of key-value pairs representing the query parameters
     */
    std::map<std::string, std::string> parseUrlQueryParameters(const std::string &url);

    /**
     * @brief Parse Multipart data from a POST request body
     *
     * @param boundary (const std::string &boundary) The boundary string used to separate parts in the multipart data
     * @param body (const std::string &body) The body of the POST request containing multipart data
     *
     * @return A vector of multipartData objects, each representing a part of the multipart data
     */
    std::vector<multipartData> parseMultipart(const std::string &boundary, const std::string &body);

    /**
     * @brief Send data to the socket using HTTP protocol
     *
     * @param data (const std::string) Data to be sent
     * @param socket (SOCKET) socket to send data to
     */
    void sendDataToHttpSocket(SOCKET socket, const std::string data);

    /**
     * @brief Send data to the socket using HTTPS protocol
     *
     * @param data (const std::string) Data to be sent
     * @param ssl (SSL *) SSL structure for sending data
     */
    void sendDataToHttpsSocket(SOCKET socket, const std::string data);

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
     * @throws Socket read failed during chunk size reception
     * @throws Failed to parse chunk size
     * @throws Failed to parse chunk size with unknown error
     * @throws Incomplete chunk data: connection closed unexpectedly or socket read error
     * @throws shutdown failed
     * @throws Invalid HTTP response format
     */
    void sendDataToHttpHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content);

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
     * @throws SSL read failed during chunk size reception
     * @throws Failed to parse chunk size with unknown error
     * @throws Incomplete chunk data: connection closed unexpectedly or SSL read error
     * @throws Invalid HTTP response format
     */
    void sendDataToHttpsHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA = true);

    /**
     * @brief Automatically executed when leaving scope
     */
    ~cppNetworkUtil();

    cppNetworkUtil();

    // 禁用拷贝构造和赋值运算符，因为 unique_ptr 不支持拷贝
    cppNetworkUtil(const cppNetworkUtil &) = delete;
    cppNetworkUtil &operator=(const cppNetworkUtil &) = delete;

    void printOpensslVersion();

    void run(int port, serverCallback *callback);

private:
    std::unique_ptr<cppNetworkUtilPimpl> pimpl_; // 不透明指针
};