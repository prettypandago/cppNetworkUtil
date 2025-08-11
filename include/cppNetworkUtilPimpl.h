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

const std::map<int, std::string> http_code = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}};

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
     * @brief parse header (mapkey: method url http_version ...)
     *
     * @param header (const std::string &) request header
     *
     * @throw Invalid request line: No space found after method
     * @throw Invalid request line: No space found after url
     * @throw Invalid request line: No \r\n found after http version
     *
     * @return parsed map request header
     */
    std::map<std::string, std::string> getParsedHeader_Pimpl(const std::string &header);

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
     * @brief Get the content size in the http request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throw Find Content-Length failed
     * @throw Parse Content-Length failed
     * @throw Parse Content-Length failed with unknown error
     * @throw Find body failed
     *
     * @return content size
     */
    int getContentSize_Pimpl(const std::string buffer);

    /**
     * @brief Calculates the size of the POST content from the given buffer.
     *
     * This function analyzes the provided buffer, which is expected to contain
     * HTTP request data, and determines the size of the POST content.
     *
     * @param buffer The input string containing the HTTP request data.
     *
     * @throw Find Content-Length failed
     * @throw Parse Content-Length failed
     * @throw Parse Content-Length failed with unknown error
     * @throw Find body failed
     * @throw Size does not meet the requirements
     *
     * @return The size of the POST content in bytes.
     */
    int getPostContentSize_Pimpl(const std::string &buffer);

    /**
     * @brief Determines the Content-Type of a POST request from the provided buffer.
     *
     * This function analyzes the given buffer, which is expected to contain the headers or body
     * of an HTTP POST request, and extracts or infers the Content-Type value.
     *
     * @param buffer The input string containing HTTP request data.
     *
     * @return A string representing the Content-Type of the POST request. Returns an empty string if the Content-Type cannot be determined.
     */
    std::string getPostContentType_Pimpl(const std::string &buffer);

    /**
     * @brief Extracts the boundary string from the given HTTP POST content buffer.
     *
     * This function parses the provided buffer, typically containing HTTP headers,
     * and retrieves the boundary value used in multipart/form-data POST requests.
     *
     * @param buffer The input string containing the HTTP POST content or headers.
     *
     * @return The extracted boundary string if found; otherwise, an empty string.
     */
    std::string getPostContentBoundary_Pimpl(const std::string &buffer);

    /**
     * @brief Get the content body in the http POST request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throw Find Content-Length failed
     * @throw Parse Content-Length failed
     * @throw Parse Content-Length failed with unknown error
     * @throw Find body failed
     * @throw Size does not meet the requirements
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
    std::string makeResponseHeader_Pimpl(responseHeaderParameters parameters);

    /**
     * @brief Make a request header
     *
     * @param parameter (requestHeaderParameters) parameter
     *
     * @return header
     */
    std::string makeRequestHeader_Pimpl(requestHeaderParameters parameter);

    /**
     * @brief Decode a URL-encoded string
     *
     * @param encodedString (const std::string &) The string to be decoded
     *
     * @return Decoded string
     */
    std::string urlDecode_Pimpl(const std::string &encodedString);

    /**
     * @brief Parses a URL-encoded form body string into a map of key-value pairs.
     *
     * This function takes a URL-encoded string (typically from an HTTP POST body)
     * and parses it into a std::map, where each key-value pair corresponds to a
     * field in the form data.
     *
     * @param postBody The URL-encoded form body as a std::string.
     *
     * @return std::map<std::string, std::string> A map containing the decoded key-value pairs.
     */
    std::map<std::string, std::string> parseUrlEncodedFormBody_Pimpl(const std::string &postBody);

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
     * @throw Invalid URL format
     *
     * @return A map of key-value pairs representing the query parameters
     */
    std::map<std::string, std::string> parseUrlQueryParameters_Pimpl(const std::string &url);

    /**
     * @brief Parses a multipart/form-data HTTP body using the specified boundary.
     *
     * This function processes the given HTTP request body, extracting each part
     * separated by the provided boundary string. Each part is parsed and stored
     * as a multipartData object, mapped by its corresponding field name.
     *
     * @param boundary The boundary string used to separate parts in the multipart body.
     * @param body The raw HTTP request body containing multipart/form-data.
     *
     * @return std::map<std::string, multipartData>
     *         A map where the key is the field name and the value is the parsed multipartData.
     */
    std::map<std::string, multipartData> parseMultipart_Pimpl(const std::string &boundary, const std::string &body);

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
     * @throw WSAStartup failed
     * @throw getaddrinfo failed
     * @throw Unable to connect to server
     * @throw Send failed
     * @throw Socket read failed during chunk size reception
     * @throw Failed to parse chunk size
     * @throw Failed to parse chunk size with unknown error
     * @throw Incomplete chunk data: connection closed unexpectedly or socket read error
     * @throw shutdown failed
     * @throw Invalid HTTP response format
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
     * @throw WSAStartup failed
     * @throw Failed to load default CA certificates
     * @throw Failed to create SSL object
     * @throw Failed to create BIO connection
     * @throw Failed to connect to server
     * @throw SSL handshake failed
     * @throw SSL read failed during chunk size reception
     * @throw Failed to parse chunk size with unknown error
     * @throw Incomplete chunk data: connection closed unexpectedly or SSL read error
     * @throw Invalid HTTP response format
     */
    void sendDataToHttpsHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA = true);

    /*
     * @brief Run the server with the specified port and callback
     *
     * @param port (int) The port number to run the server on
     * @param callback (serverCallback *) The callback to handle server events
     *
     * @throw WSAStartup failed
     * @throw Unable to create SSL context
     * @throw Unable to load certificate PUBLIC KEY
     * @throw Unable to load private key PRIVATE KEY
     * @throw Private key does not match the certificate
     * @throw Create socket failed
     * @throw Setsockopt failed
     * @throw Bind failed
     * @throw Listen failed
     * @throw Unable to get the number of CPU cores
     */
    void run_Pimpl(int port, serverCallback *callback);

    /**
     * @brief Print the cppNetowrkUtil version
     */
    void print_cppNetworkUtilVersion_Pimpl();

    /**
     * @brief Print the openSSL version
     */
    void print_opensslVersion_Pimpl();

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
     * @throw Invalid server socket
     * @throw No callback provided to process the request
     *
     * This function runs in a loop, accepting incoming client connections and processing their requests.
     * It uses the provided callback to notify the user of received data.
     */
    void process(SOCKET server_socket, serverCallback *callback);
};