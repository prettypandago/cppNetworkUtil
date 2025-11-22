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
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "defines.h"
#include "log.h"

#include "cppNetworkUtil.h"
#include "threadPool.h"

// OpenSSL 相关前向声明
struct ssl_ctx_st;
struct ssl_st;
struct bio_st; // 用于 OpenSSL 的 BIO

const std::unordered_map<int, std::string> http_code = {{100, "Continue"},
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

// 常见的 MIME 类型映射
const std::unordered_map<std::string, std::string> mimeTypeMap = {
    // 文本类型 (text)
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    // JavaScript 的标准推荐类型是 application/javascript
    {"js", "application/javascript"},
    {"mjs", "application/javascript"},
    {"json", "application/json"},
    {"xml", "application/xml"},
    {"txt", "text/plain"},
    {"csv", "text/csv"},

    // 图像类型 (image)
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"svg", "image/svg+xml"},
    {"ico", "image/vnd.microsoft.icon"},
    {"webp", "image/webp"},

    // 应用/通用类型 (application)
    {"pdf", "application/pdf"},
    // 通用二进制流，用于未知或需要下载的文件
    {"bin", "application/octet-stream"},
    {"zip", "application/zip"},
    {"gz", "application/gzip"},

    // 音频类型 (audio)
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"ogg", "audio/ogg"},
    {"aac", "audio/aac"},

    // 视频类型 (video)
    {"mp4", "video/mp4"},
    {"webm", "video/webm"},
    {"ogv", "video/ogg"},

    // 字体类型 (font)
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},

    // Microsoft Office (Office Open XML 格式)
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
};

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
        bool is_keep_alive_connection;
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
    std::unordered_map<SOCKET, std::unordered_map<int, clientConnectionInfo_Pimpl>>
        client_connections; // 存储客户端连接信息

    std::unordered_map<std::string, std::vector<routeInfo>> handlers; // 路由表

    // // 专门用于固定方法的哈希表
    // std::unordered_map<std::string, std::vector<routeInfo>> fixed_method_handlers;

    // // 专门用于正则表达式方法的向量（包括 *）
    // std::vector<std::pair<std::regex, std::vector<routeInfo>>> regex_method_handlers;

    /**
     * @brief Determine whether a string is a regular expression
     *
     * @param s (const std::string &) string
     *
     * @return Is regex pattern
     */
    bool isRegexPattern_Pimpl(const std::string &s);

    /**
     * @brief Determine whether a given IP address (IPv4 or IPv6) is a local area network (LAN) (private) address.
     * * Private address ranges are based on RFC 1918 (IPv4), RFC 3927 (IPv4 Link-Local),
     * RFC 4193 (IPv6 ULA), and RFC 4291 (IPv6 Link-Local).
     * * @param ip_str (const std::string &) IP address string to check
     * @return Is local ip address
     */
    bool isLocalIpAddress_Pimpl(const std::string &ip_str);

    /**
     * @brief parse header (mapkey: method url http_version ...)
     *
     * @param header (const std::string &) request header
     *
     * @throw Invalid request line: No space found after method
     * @throw Invalid request line: No space found after url
     * @throw Invalid request line: No \r\n found after http version
     *
     * @return Parsed unordered_map request header
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
     * @brief get the Transfer-Encoding
     *
     * @param headers (const std::string &) The HTTP request header string
     *
     * @return transfer encoding value
     */
    std::string getTransferEncodingValue_Pimpl(const std::string &headers);

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
     * @brief Retrieves the standard HTTP status text for a given status code.
     *
     * This function looks up the provided HTTP status code in a predefined unordered_map
     * and returns the corresponding status text. If the status code is not found,
     * std::nullopt is returned.
     *
     * @param status_code The HTTP status code (e.g., 200, 404).
     *
     * @return std::optional<std::string> The standard textual description for the provided HTTP status code,
     * or std::nullopt if the status code is not recognized.
     */
    std::optional<std::string> getHttpCodeText_Pimpl(int status_code);

    /**
     * @brief Retrieves the MIME type associated with a given file extension.
     *
     * This function takes a file extension as input and returns the corresponding
     * MIME type as a string, if it exists. If the MIME type cannot be determined,
     * std::nullopt is returned.
     *
     * @param file_extension The file extension (e.g., "jpg", "html") for which to retrieve the MIME type.
     *
     * @return std::optional<std::string> The MIME type string if found, otherwise std::nullopt.
     */
    std::optional<std::string> getMimeType_Pimpl(const std::string &file_extension);

    /**
     * @brief Make a response header
     *
     * @param status_code (int) HTTP status code
     * @param parameters (std::unordered_map<std::string, std::string>) parameters
     *
     * @throw Missing required fields
     *
     * @return response header
     */
    std::string makeResponseHeader_Pimpl(int status_code, std::unordered_map<std::string, std::string> parameters);

    /**
     * @brief Make a request header
     *
     * @param parameters (std::unordered_map<std::string, std::string>) parameters
     *
     * @throw Missing required fields
     *
     * @return request header
     */
    std::string makeRequestHeader_Pimpl(std::unordered_map<std::string, std::string> parameters);

    /**
     * @brief Decode a URL-encoded string
     *
     * @param encodedString (const std::string &) The string to be decoded
     *
     * @return Decoded string
     */
    std::string urlDecode_Pimpl(const std::string &encodedString);

    /**
     * @brief Parses a URL-encoded form body string into a unordered_map of key-value pairs.
     *
     * This function takes a URL-encoded string (typically from an HTTP POST body)
     * and parses it into a std::unordered_map, where each key-value pair corresponds to a
     * field in the form data.
     *
     * @param postBody The URL-encoded form body as a std::string.
     *
     * @return std::unordered_map<std::string, std::string> A unordered_map containing the decoded key-value pairs.
     */
    std::unordered_map<std::string, std::string> parseUrlEncodedFormBody_Pimpl(const std::string &postBody);

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
     * @return A unordered unordered_map  of key-value pairs representing the query parameters
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
     * @return std::unordered_map<std::string, multipartData>
     *         A unordered_map where the key is the field name and the value is the parsed multipartData.
     */
    std::unordered_map<std::string, multipartData> parseMultipart_Pimpl(const std::string &boundary,
                                                                        const std::string &body);

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
     * @param request_count (int) The request count for keep-alive connections
     * @param data (const std::string &) Data to be sent
     */
    void sendDataToHttpsSocket_Pimpl(SOCKET socket, int request_count, const std::string &data);

    /**
     * @brief Send data to the socket using HTTPS protocol
     *
     * @param ssl (ssl_st *) SSL structure for sending data
     * @param data (const std::string &) Data to be sent
     */
    void sendDataToHttpsSocket_Pimpl(ssl_st *ssl, const std::string &data);

    /**
     * @brief Send data to the socket (auto select HTTP or HTTPS)
     *
     * @param socket (SOCKET) The socket to send data to
     * @param request_count (int) The request count for the current connection
     * @param data (const std::string &) Data to be sent
     */
    void sendDataToSocket_Pimpl(SOCKET socket, int request_count, const std::string &data);

    /**
     * @brief Send data to the socket in chunks
     *
     * @param socket (SOCKET) The socket to send data to
     * @param reuqest_count (int) The request count for the current connection
     * @param data (const std::string &) Data to be sent
     */
    void sendDataChunkToSocket_Pimpl(SOCKET socket, int request_count, const std::string &data);

    /**
     * @brief Send data to the host using HTTP protocol
     *
     * @param host (const std::string &) Host address
     * @param path (const std::string &) Path to the resource
     * @param port (int) Port number
     * @param request_header (const std::unordered_map<std::string, std::string> &) Request header to be sent
     * @param header (std::string &) Header to be filled with the response header
     * @param content (std::string &) Content to be filled with the response content
     *
     * @throw WSAStartup failed
     * @throw getaddrinfo failed
     * @throw Unable to connect to server
     * @throw Socket read failed during chunk size reception
     * @throw Failed to parse chunk size
     * @throw Failed to parse chunk size with unknown error
     * @throw Incomplete chunk data: connection closed unexpectedly or socket read error
     * @throw shutdown failed
     * @throw Invalid HTTP response format
     */
    void sendDataToHttpHost_Pimpl(const std::string &host, const std::string &path, int port,
                                  const std::unordered_map<std::string, std::string> &request_header,
                                  std::string &header, std::string &content);

    /**
     * @brief Send data to the host using HTTPS protocol
     *
     * @param host (const std::string &) Host address
     * @param path (const std::string &) Path to the resource
     * @param port (int) Port number
     * @param request_header (const std::string &) Request header to be sent
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
    void sendDataToHttpsHost_Pimpl(const std::string &host, const std::string &path, int port,
                                   const std::unordered_map<std::string, std::string> &request_header,
                                   std::string &header, std::string &content, bool enable_CA = true);

    /**
     * @brief Decode chunked transfer encoding response
     *
     * @param current_chunk_buffer (std::string &) Buffer to store the current chunk data
     * @param content (std::string &) Content to be filled with the decoded response content
     * @param read_func (std::function<int(char *, int)>) Function to read data from the socket or SSL
     *
     * @throw Socket read failed during chunk size reception
     * @throw Failed to parse chunk size
     * @throw Failed to parse chunk size with unknown error
     * @throw Incomplete chunk data: connection closed unexpectedly or socket/SSL read error
     */
    void decodeChunkedResponse_Pimpl(std::string &current_chunk_buffer, std::string &content,
                                     std::function<int(char *, int)> read_func);

    /*
     * @brief Run the server with the specified port and callback
     *
     * @param http_port (int) The http port number to run the server on
     * @param https_port (int) The https port number to run the server on
     * @param behavior_mode (int) The server behavior mode (0: Redirect HTTP request to HTTPS, 1: Handling HTTP and
     * HTTPS requests)
     * @param ip_protocol_mode (int) The IP protocol mode (0: IPv4, 1: IPv6, 2: both)
     * @param cert_path (std::string) The path to the SSL certificate file
     * @param key_path (std::string) The path to the SSL private key file
     * @param print_listen_info (bool) Whether to print listen info
     * @param timeout (std::uint32_t) Socket timeout in milliseconds
     * @param max_request_count (int) Maximum request count for keep-alive connections
     *
     * @throw WSAStartup failed
     * @throw At least one port must be enabled
     * @throw Unable to create SSL context
     * @throw Unable to load certificate
     * @throw Unable to load private key
     * @throw Private key does not match the certificate
     */
    void run_Pimpl(int http_port = DEFAULT_HTTP_SERVER_PORT, int https_port = DEFAULT_HTTPS_SERVER_PORT,
                   int behavior_mode = BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS,
                   int ip_protocol_mode = IP_PROTOCOL_MODE_IPV4_AND_IPV6_BOTH,
                   std::string cert_path = DEFAULT_CERT_PATH, std::string key_path = DEFAULT_KEY_PATH,
                   bool print_listen_info = ENABLE_PRINT_LISTEN_INFO, std::uint32_t timeout = DEFAULT_TIMEOUT_MS,
                   int max_request_count = DEFAULT_MAX_REQUEST_COUNT);

    /**
     * @brief Print the cppNetowrkUtil version
     */
    void print_cppNetworkUtilVersion_Pimpl();

    /**
     * @brief Print the openSSL version
     */
    void print_opensslVersion_Pimpl();

    /**
     * @brief Determine whether a pattern contains template
     *
     * @param pattern (const std::string &) pattern
     *
     * @return Contains template
     */
    bool containsTemplate(const std::string &pattern);

    /**
     * @brief Determine whether a pattern is a pure regex
     *
     * @param pattern (const std::string &) pattern
     *
     * @return Is pure regex
     */
    bool isPureRegex(const std::string &pattern);

    /**
     * @brief Parse template path and build standard regex
     *
     * @param pattern (const std::string &) template path
     * @param param_names (std::vector<std::string> &) parameter names
     *
     * @throw Template error: Variable name cannot be empty
     * @throw Template error: Missing closing brace '}'
     *
     * @return standard regex string
     */
    std::string parseAndCompileTemplate(const std::string &pattern, std::vector<std::string> &param_names);

    /**
     * @brief Make regex string from path pattern
     *
     * @param path_pattern (const std::string &) path pattern
     * @param param_names (std::vector<std::string> &) parameter names
     *
     * @return regex string
     */
    std::string makeRagexString_Pimpl(const std::string &path_pattern, std::vector<std::string> &param_names);

    /**
     * @brief Register a route handler for a specific HTTP method and path pattern.
     *
     * This function allows you to define how the server should respond to requests
     *
     * @param method The HTTP method (e.g., "GET", "POST") for which the handler is registered.
     * @param status_code The HTTP status code for which the handler is registered.
     * @param path_pattern The path pattern to match against incoming requests.
     * @param handler The function or callable object that will handle the request.
     */
    void on_Pimpl(const std::string &method, const int &status_code, const std::string &path_pattern,
                  const routeHandler &handler);

    /**
     * @brief Unregister route handlers.
     *
     * This function removes all route handlers. If no matching handlers are found, the function does nothing.
     *
     * @param method The HTTP method (e.g., "GET", "POST") for which the handler is registered.
     * @param status_code The HTTP status code for which the handler is registered.
     * @param path_pattern The path pattern to match against incoming requests.
     */
    void off_Pimpl(const std::string &method, const int &status_code, const std::string &path_pattern);

    /**
     * @brief Invokes the error handler for a specific HTTP status code.
     *
     * This function is called when an error occurs during request processing,
     * allowing the server to respond with a custom error message or handling logic.
     *
     * @param status_code The HTTP status code indicating the type of error (e.g., 404, 500).
     * @param req The request context containing information about the incoming request.
     * @param res The response context to be modified by the error handler.
     */
    void invokeErrorHandler_Pimpl(int status_code, const requestContext &req, responseContext &res);

    /**
     * @brief Handles an incoming request and generates a corresponding response.
     *
     * This function processes the provided request context and returns a response context
     * containing the results of the request handling. It is intended to be used internally
     * within the implementation (Pimpl) of the network utility.
     *
     * @param req The context of the incoming request to be handled.
     *
     * @return std::optional<responseContext> The context containing the response to the request.
     */
    std::optional<responseContext> handleRequest_Pimpl(const requestContext &req);

  private:
    ssl_ctx_st *ssl_ctx_server; // 服务器端 SSL 上下文
    ssl_ctx_st *ssl_ctx_client; // 客户端 SSL 上下文
    ssl_st *ssl;                // 使用 BIO 方式进行网络操作

    /**
     * @brief Ensure that the buffer contains data up to the specified target size.
     *
     * This function reads data from the given socket and appends it to the provided buffer
     * until the buffer reaches the target size. It returns true if the target size is reached,
     * otherwise false.
     *
     * @param sock (SOCKET) The socket from which to read data.
     * @param ssl_conn (ssl_st *) The SSL connection object for secure connections (can be nullptr for non-SSL).
     * @param buffer (std::string &) The buffer to which data will be appended.
     * @param target_size (size_t) The target size that the buffer should reach.
     *
     * @return true if the buffer reaches the target size, false otherwise.
     */
    bool ensureBuffer_Pimpl(SOCKET sock, ssl_st *ssl_conn, std::string &buffer, size_t target_size);

    /**
     * @brief Ensure that the buffer contains data up to the specified target string.
     *
     * This function reads data from the given socket and appends it to the provided buffer
     * until the target string is found within the buffer. It returns true if the target
     * string is found, otherwise false.
     *
     * @param sock (SOCKET) The socket from which to read data.
     * @param ssl_conn (ssl_st *) The SSL connection object for secure connections (can be nullptr for non-SSL).
     * @param buffer (std::string &) The buffer to which data will be appended.
     * @param target (const std::string &) The target string to search for in the buffer.
     *
     * @return true if the target string is found in the buffer, false otherwise.
     */
    bool ensureBufferUntil_Pimpl(SOCKET sock, ssl_st *ssl_conn, std::string &buffer, const std::string &target);

    /**
     * @brief Parse chunked transfer encoding body from the buffer.
     *
     * This function processes the provided buffer containing chunked transfer encoding data
     * and extracts the complete body into the outBody string. It returns true if the parsing
     * is successful, otherwise false.
     *
     * @param sock (SOCKET) The socket from which to read data.
     * @param ssl_conn (ssl_st *) The SSL connection object for secure connections (can be nullptr for non-SSL).
     * @param buffer (std::string &) The buffer containing chunked transfer encoding data.
     * @param outBody (std::string &) The output string where the parsed body will be stored.
     *
     * @return true if the chunked body is successfully parsed, false otherwise.
     */
    bool parseChunkedBody_Pimpl(SOCKET sock, ssl_st *ssl_conn, std::string &buffer, std::string &outBody);

    /*
     * @brief Create and bind a socket to the specified port and IP protocol mode

     * @param port (int) The port number to bind the socket to
     * @param ip_protocol_family (int) The IP protocol family (AF_INET for IPv4, AF_INET6 for IPv6)
     * @param is_ipv6_only (bool) set is ipv6 only
     *
     * @throw Create socket failed
     * @throw Setsockopt failed
     * @throw Bind failed
     * @throw Listen failed
     *
     * @return The created and bound socket
     */
    SOCKET createAndBindSocket_Pimpl(int port, int ip_protocol_family, bool is_ipv6_only);

    /**
     * @brief Accepts incoming connections on the specified server socket and handles them according to the provided
     * parameters.
     *
     * @param server_socket The server socket to accept connections from.
     * @param enable_https If true, enables HTTPS support for incoming connections.
     * @param http_port The port number to use for HTTP connections. Defaults to DEFAULT_HTTP_SERVER_PORT.
     * @param https_port The port number to use for HTTPS connections. Defaults to DEFAULT_HTTPS_SERVER_PORT.
     * @param behavior_mode The behavior mode for handling requests (e.g., redirect HTTP to HTTPS). Defaults to
     * BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS.
     * @param timeout The timeout duration in milliseconds for socket operations.
     * @param max_request_count The maximum number of requests to handle for keep-alive connections.
     */
    void acceptScocket_Pimpl(SOCKET server_socket, bool enable_https, int http_port = DEFAULT_HTTP_SERVER_PORT,
                             int https_port = DEFAULT_HTTPS_SERVER_PORT,
                             int behavior_mode = BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS,
                             std::uint32_t timeout = DEFAULT_TIMEOUT_MS,
                             int max_request_count = DEFAULT_MAX_REQUEST_COUNT);

    /**
     * @brief Processes a client connection on the server socket.
     *
     * Handles the incoming client connection, optionally using SSL/TLS if enabled, and processes
     * the request according to the specified behavior mode. Supports both HTTP and HTTPS protocols.
     *
     * @param client_socket      The client socket descriptor.
     * @param client_address     The address information of the connected client.
     * @param ssl_conn           Pointer to the SSL connection structure (used if HTTPS is enabled).
     * @param enable_https       Flag indicating whether HTTPS is enabled.
     * @param http_port          The port number for HTTP connections (default: DEFAULT_HTTP_SERVER_PORT).
     * @param https_port         The port number for HTTPS connections (default: DEFAULT_HTTPS_SERVER_PORT).
     * @param behavior_mode      The behavior mode for processing requests (default:
     * BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS).
     * @param timeout            The timeout duration in milliseconds for socket operations (default:
     * DEFAULT_TIMEOUT_MS).
     * @param max_request_count  The maximum number of requests to handle for keep-alive connections (default:
     */
    void process(SOCKET client_socket, struct sockaddr_storage client_address, ssl_st *ssl_conn, bool enable_https,
                 int http_port = DEFAULT_HTTP_SERVER_PORT, int https_port = DEFAULT_HTTPS_SERVER_PORT,
                 int behavior_mode = BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS,
                 std::uint32_t timeout = DEFAULT_TIMEOUT_MS, int max_request_count = DEFAULT_MAX_REQUEST_COUNT);
};
