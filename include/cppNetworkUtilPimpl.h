#pragma once

#include <iostream>

#ifdef _WIN32

#include <shlobj.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#else

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef int SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define WSAGetLastError() errno

#endif

#include <algorithm>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "defines.h"
#include "log.h"

#include "cppNetworkUtil.h"
#include "threadPool.h"

// OpenSSL 相关前向声明
struct ssl_ctx_st;
struct ssl_st;
struct bio_st; // 用于 OpenSSL 的 BIO

const std::map<int, std::string> http_code = {{100, "Continue"},
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

    struct clientConnectionInfo_Pimpl
    {
        SOCKET client_socket;
        ssl_st *ssl;
        bool is_https_connection;
        std::string ip;
        int port;
        std::string family;
        std::string request_data;
        std::string request_header;
        std::string request_content;
        std::unordered_map<std::string, std::string> path_params;
        std::unordered_map<std::string, std::string> query_params;
        std::unordered_map<std::string, std::string> parsed_request_headers;
    };
    std::map<SOCKET, clientConnectionInfo_Pimpl> client_connections; // 存储客户端连接信息

    std::unordered_map<std::string, std::vector<routeInfo>> routes; // 储存路由信息
    std::unordered_map<int, routeHandler> error_handlers;           // 储存错误路由信息

    /**
     * @brief parse header (mapkey: method url http_version ...)
     *
     * @param header (const std::string &) request header
     *
     * @throw Invalid request line: No space found after method
     * @throw Invalid request line: No space found after url
     * @throw Invalid request line: No \r\n found after http version
     *
     * @return parsed unordered_map request header
     */
    std::unordered_map<std::string, std::string> getParsedHeader_Pimpl(const std::string &header);

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
     * @return A string representing the Content-Type of the POST request. Returns an empty string if the Content-Type
     * cannot be determined.
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
     * @brief Returns the standard textual description for a given HTTP status code.
     *
     * This function takes an integer representing an HTTP status code (e.g., 200, 404)
     * and returns the corresponding standard reason phrase as a string (e.g., "OK", "Not Found").
     *
     * @param code The HTTP status code to look up.
     *
     * @return std::string The standard textual description for the provided HTTP status code.
     */
    std::string getHttpCodeText_Pimpl(int code);

    /**
     * @brief Make a response header
     *
     * @param status (int) HTTP status code
     * @param parameters (std::unordered_map<std::string, std::string>) parameters
     *
     * @throw Missing required fields
     *
     * @return response header
     */
    std::string makeResponseHeader_Pimpl(int status, std::unordered_map<std::string, std::string> parameters);

    /**
     * @brief Make a request header
     *
     * @param parameters (std::map<std::string, std::string>) parameters
     *
     * @throw Missing required fields
     *
     * @return request header
     */
    std::string makeRequestHeader_Pimpl(std::map<std::string, std::string> parameters);

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
     * @brief Cut url path
     *
     * @param (std::string) url path
     *
     * @return a cut std::vector<std::string>
     */
    std::vector<std::string> cutUrlPath_Pimpl(std::string path);

    /**
     * @brief Parse URL parameters from URL query string
     *
     * @param url (std::string) The URL to parse
     *
     * @throw Invalid URL format
     *
     * @return A unordered map  of key-value pairs representing the query parameters
     */
    std::unordered_map<std::string, std::string> parseUrlQueryParameters_Pimpl(const std::string &url);

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
    void sendDataToHttpHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header,
                                  std::string &content);

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
    void sendDataToHttpsHost_Pimpl(const std::string &host, const std::string &path, int port, std::string &header,
                                   std::string &content, bool enable_CA = true);

    /*
     * @brief Run the server with the specified port and callback
     *
     * @param http_port (int) The http port number to run the server on
     * @param https_port (int) The https port number to run the server on
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
    void run_Pimpl(int http_port = DEFAULT_HTTP_SERVER_PORT, int https_port = DEFAULT_HTTPS_SERVER_PORT);

    /**
     * @brief Print the cppNetowrkUtil version
     */
    void print_cppNetworkUtilVersion_Pimpl();

    /**
     * @brief Print the openSSL version
     */
    void print_opensslVersion_Pimpl();

    /**
     * @brief Registers a route handler for a specific HTTP method and path pattern.
     *
     * Associates the given handler with the specified HTTP method and path pattern.
     * When a request matches the method and path pattern, the corresponding handler will be invoked.
     *
     * @param method The HTTP method (e.g., "GET", "POST") for which the route is registered.
     * @param path_pattern The path pattern to match incoming requests (e.g., "/api/items/:id").
     * @param handler The function or callable object to handle requests matching the method and path pattern.
     */
    void registerRoute_Pimpl(const std::string &method, const std::string &path_pattern, routeHandler handler);

    /**
     * @brief Handles an incoming request and generates a corresponding response.
     *
     * This function processes the provided request context and returns a response context
     * containing the results of the request handling. It is intended to be used internally
     * within the implementation (Pimpl) of the network utility.
     *
     * @param req The context of the incoming request to be handled.
     *
     * @return responseContext The context containing the response to the request.
     */
    responseContext handleRequest_Pimpl(const requestContext &req);

    /**
     * @brief Sets a custom error handler for a specific HTTP status code.
     *
     * Associates the given route handler with the specified status code.
     * When an error with the provided status code occurs, the handler will be invoked.
     *
     * @param status_code The HTTP status code to handle (e.g., 404, 500).
     * @param handler The function or callable object to handle the error.
     */
    void setErrorHandler_Pimpl(int status_code, routeHandler handler);

  private:
    ssl_ctx_st *ssl_ctx_server; // 服务器端 SSL 上下文
    ssl_ctx_st *ssl_ctx_client; // 客户端 SSL 上下文
    ssl_st *ssl;                // 使用 BIO 方式进行网络操作

    // 指向父 cppNetworkUtil 实例的指针，用于在回调中传递给用户
    cppNetworkUtil *parent_util_;

    /*
     * @brief Process incoming connections and handle requests
     *
     * @param server_socket (SOCKET) The server socket to accept connections from
     * @param enable_https (bool) Whether to enable HTTPS support
     * @param http_port (int) The HTTP port to listen on
     * @param https_port (int) The HTTPS port to listen on
     *
     * @throw Invalid server socket
     *
     * This function runs in a loop, accepting incoming client connections and processing their requests.
     * It uses the provided callback to notify the user of received data.
     */
    void process(SOCKET server_socket, bool enable_https, int http_port = DEFAULT_HTTP_SERVER_PORT,
                 int https_port = DEFAULT_HTTPS_SERVER_PORT);
};
