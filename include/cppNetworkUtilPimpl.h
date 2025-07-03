#pragma once

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

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

#include "defines.h"
#include "log.h"

#include "cppNetworkUtil.h"
#include "threadPool.h"

// OpenSSL 相关前向声明
struct ssl_ctx_st;
struct ssl_st;
struct bio_st; // 用于 OpenSSL 的 BIO

// 向前申明
class serverCallback; // 回调接口

class cppNetworkUtilPimpl
{
public:
    cppNetworkUtilPimpl();
    ~cppNetworkUtilPimpl();

    struct clientConnectionInfo
    {
        ssl_st *ssl;
        std::string ip;
        int port;
        std::string request_data;
        std::string request_header;
        std::string request_content;
    };
    std::map<SOCKET, clientConnectionInfo> client_connections; // 存储客户端连接信息

    /**
     * @brief Get the method in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throws Invalid request line: No space found after method
     *
     * @return method
     */
    std::string getHeaderMethod_Pimpl(const std::string buffer);

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
    std::string getGetHeaderUrl_Pimpl(const std::string buffer);

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
    int getContentSize_Pimpl(const std::string buffer);

    /**
     * @brief Get the value of a specific header field in the HTTP request header
     *
     * @param headers (const std::string &) The HTTP request header string
     * @param key (const std::string &) The key of the header field to retrieve
     *
     * @return The value of the specified header field, or an empty string if not found
     */
    std::string getHeaderValue_Pimpl(const std::string &headers, const std::string &key);

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
    std::string getPostContentBody_Pimpl(const std::string buffer);

    /**
     * @brief Make a request header
     *
     * @param parameters (headerParameters) parameter
     *
     * @return header
     */
    std::string buildResponseHeader_Pimpl(responseHeaderParameters parameters);

    /**
     * @brief Make a request header
     *
     * @param parameter (requestHeaderParameters) parameter
     *
     * @return header
     */
    std::string buildRequestHeader_Pimpl(requestHeaderParameters parameter);

    /**
     * @brief Decode a URL-encoded string
     *
     * @param encodedString (const std::string &) The string to be decoded
     *
     * @return Decoded string
     */
    std::string urlDecode_Pimpl(const std::string &encodedString);

    /**
     * @brief Parse URL parameters from a RESTful API style URL
     *
     * @param url (std::string) The URL to parse
     *
     * @return A vector of strings representing the parameters in the URL
     */
    std::vector<std::string> getURLParameterRestfulapi_Pimpl(std::string url);

    /**
     * @brief Parse URL parameters from URL query string
     *
     * @param url (std::string) The URL to parse
     *
     * @throws Invalid URL format
     *
     * @return A map of key-value pairs representing the query parameters
     */
    std::map<std::string, std::string> parseUrlQueryParameters_Pimpl(const std::string &url);

    /**
     * @brief Parse Multipart data from a POST request body
     *
     * @param boundary (const std::string &boundary) The boundary string used to separate parts in the multipart data
     * @param body (const std::string &body) The body of the POST request containing multipart data
     *
     * @return A vector of multipartData objects, each representing a part of the multipart data
     */
    std::vector<multipartData> parseMultipart_Pimpl(const std::string &boundary, const std::string &body);

    /**
     * @brief Send data to the socket using HTTP protocol
     *
     * @param socket (SOCKET) The socket to send data to
     * @param data (const std::string &) Data to be sent
     */
    void sendDataToHttpSocket_Pimpl(SOCKET socket, const std::string &data);

    /**
     * @brief Send data to the socket using HTTPS protocol
     *
     * @param socket (SOCKET) The socket to send data to
     * @param data (const std::string &) Data to be sent
     */
    void sendDataToHttpsSocket_Pimpl(SOCKET socket, const std::string &data);

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
    void sendDataToHttpHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header, std::string &content);

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
    void sendDataToHttpsHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA = true);

    /**
     * @brief Print the OpenSSL version
     */
    void printOpensslVersion_Pimpl();

    /*
     * @brief Run the server with the specified port and callback
     *
     * @param port (int) The port number to run the server on
     * @param callback (serverCallback *) The callback to handle server events
     *
     * @throws WSAStartup failed
     * @throws Unable to create SSL context
     * @throws Unable to load certificate PUBLIC KEY
     * @throws Unable to load private key PRIVATE KEY
     * @throws Private key does not match the certificate
     * @throws Create socket failed
     * @throws Setsockopt failed
     * @throws Bind failed
     * @throws Listen failed
     * @throws Unable to get the number of CPU cores
     */
    void run_Pimpl(int port, serverCallback *callback);

private:
    ssl_ctx_st *ssl_ctx_server; // 服务器端 SSL 上下文
    ssl_ctx_st *ssl_ctx_client; // 客户端 SSL 上下文
    ssl_st *ssl;                // 使用 BIO 方式进行网络操作

    // 指向父 cppNetworkUtil 实例的指针，用于在回调中传递给用户
    cppNetworkUtil *parent_util_;

    /*
     * @brief Process incoming connections and handle requests
     *
     * @param server_socket (SOCKET) The server socket to accept connections on
     * @param callback (serverCallback *) The callback to handle server events
     *
     * @throws Invalid server socket
     * @throws No callback provided to process the request
     *
     * This function runs in a loop, accepting incoming client connections and processing their requests.
     * It uses the provided callback to notify the user of received data.
     */
    void process(SOCKET server_socket, serverCallback *callback);
};