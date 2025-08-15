#pragma once

// Forward declaration of implementation class
class cppNetworkUtilPimpl;

// include
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

#include <algorithm> // For std::min
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>    // For std::ifstream
#include <functional> // For std::function
#include <future>     // For std::future, std::packaged_task
#include <map>
#include <memory> //For std::unique_ptr
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "defines.h"
#include "log.h"

// cppNetworkUtil
class cppNetworkUtil
{
  public:
    /**
     * @brief Get the client connection info
     *
     * @param client_socket (SOCKET) Client socket
     *
     * @return Client connection info
     */
    requestContext getClientConnectionsInfo(SOCKET client_socket);

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
    std::unordered_map<std::string, std::string> getParsedHeader(const std::string &header);

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
    int getContentSize(const std::string buffer);

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
    int getPostContentSize(const std::string &buffer);

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
    std::string getPostContentType(const std::string &buffer);

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
    std::string getPostContentBoundary(const std::string &buffer);

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
    std::string getPostContentBody(const std::string buffer);

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
    std::string getHttpCodeText(int code);

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
    std::string makeResponseHeader(int status, std::unordered_map<std::string, std::string> parameters);

    /**
     * @brief Make a request header
     *
     * @param parameters (std::map<std::string, std::string> parameters) parameters
     *
     * @throw Missing required fields
     *
     * @return request header
     */
    std::string makeRequestHeader(std::map<std::string, std::string> parameters);

    /**
     * @brief Decode a URL-encoded string
     *
     * @param encodedString (const std::string &) The string to be decoded
     *
     * @return Decoded string
     */
    std::string urlDecode(const std::string &encodedString);

    /**
     * @brief Parses a URL-encoded form body string into a map of key-value pairs.
     *
     * This function takes a URL-encoded string (typically from an HTTP POST body)
     * and parses it into a std::map, where each key-value pair corresponds to a
     * field in the form data.
     *
     * @param encoded_string The URL-encoded form body as a std::string.
     *
     * @return std::map<std::string, std::string> A map containing the decoded key-value pairs.
     */
    std::map<std::string, std::string> parseUrlEncodedFormBody(const std::string &encoded_string);

    /**
     * @brief Cut url path
     *
     * @param (std::string) url path
     *
     * @return a cut std::vector<std::string>
     */
    std::vector<std::string> cutUrlPath(std::string path);

    /**
     * @brief Parse URL parameters from URL query string
     *
     * @param url (std::string) The URL to parse
     *
     * @throw Invalid URL format
     *
     * @return A unordered map of key-value pairs representing the query parameters
     */
    std::unordered_map<std::string, std::string> parseUrlQueryParameters(const std::string &url);

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
    std::map<std::string, multipartData> parseMultipart(const std::string &boundary, const std::string &body);

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
    void sendDataToHttpHost(const std::string &host, const std::string &path, int port, std::string &header,
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
    void sendDataToHttpsHost(const std::string &host, const std::string &path, int port, std::string &header,
                             std::string &content, bool enable_CA = true);

    /**
     * @brief Automatically executed when leaving scope
     */
    ~cppNetworkUtil();

    /**
     * @brief Constructor
     */
    cppNetworkUtil();

    // Disable copy constructor and assignment operator
    cppNetworkUtil(const cppNetworkUtil &) = delete;
    cppNetworkUtil &operator=(const cppNetworkUtil &) = delete;

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
    void run(int http_port = DEFAULT_HTTP_SERVER_PORT, int https_port = DEFAULT_HTTPS_SERVER_PORT);

    /**
     * @brief Print the openSSL version
     */
    void print_opensslVersion();

    /**
     * @brief Print the cppNetworkUtil version
     */
    void print_cppNetworkUtilVersion();

    /**
     * @brief Register a route handler for a specific HTTP method and path pattern.
     *
     * This function allows you to define how the server should respond to requests
     * that match a specific HTTP method (e.g., GET, POST) and a path pattern.
     *
     * @param method The HTTP method (e.g., "GET", "POST") for which the handler is registered.
     * @param path_pattern The path pattern to match against incoming requests.
     * @param handler The function or callable object that will handle the request.
     */
    void on(const std::string &method, const std::string &path_pattern, routeHandler handler);

    /**
     * @brief Register a route handler for a specific HTTP method and status code.
     *
     * This function allows you to define how the server should respond to requests
     * that match a specific HTTP method and status code.
     *
     * @param method The HTTP method (e.g., "GET", "POST") for which the handler is registered.
     * @param status_code The HTTP status code for which the handler is registered.
     * @param handler The function or callable object that will handle the request.
     */
    void on(const std::string &method, int status_code, routeHandler handler);

  private:
    std::unique_ptr<cppNetworkUtilPimpl> pimpl_; // pimpl implementation pointer
};