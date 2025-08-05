#pragma once

// Forward declaration of implementation class
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
    int port = DEFAULT_SERVER_PORT; // server listen port

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
     * @brief Get the client connections request data
     *
     * @param client_socket (SOCKET) Client socket
     *
     * @return Client request data
     */
    std::string get_client_connections_request_data(SOCKET client_socket);

    /**
     * @brief Get the client connections recv headers
     *
     * @param client_socket (SOCKET) Client socket
     *
     * @return Client headers
     */
    std::map<std::string, std::string> get_client_connections_request_headers(SOCKET client_socket);

    /**
     * @brief Get the method in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throw Invalid request line: No space found after method
     *
     * @return method
     */
    std::string getHeaderMethod(const std::string buffer);

    /**
     * @brief Get the url in the GET request header
     *
     * @param buffer (const std::string) Received string
     *
     * @throw Invalid request line: No space found after method
     * @throw Invalid request line: No space found after url
     *
     * @return url
     */
    std::string getGetHeaderUrl(const std::string buffer);

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
     * @brief Get the value of a specific header field in the HTTP request header
     *
     * @param headers (const std::string &) The HTTP request header string
     * @param key (const std::string &) The key of the header field to retrieve
     *
     * @return The value of the specified header field, or an empty string if not found
     */
    std::string getHeaderValue(const std::string &headers, const std::string &key);

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
     * @return A string representing the Content-Type of the POST request. Returns an empty string if the Content-Type cannot be determined.
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
     * @brief Make a request header
     *
     * @param parameters (headerParameters) parameter
     *
     * @return header
     */
    std::string makeResponseHeader(responseHeaderParameters parameters);

    /**
     * @brief Make a request header
     *
     * @param parameter (requestHeaderParameters) parameter
     *
     * @return header
     */
    std::string makeRequestHeader(requestHeaderParameters parameter);

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
     * @throw Invalid URL format
     *
     * @return A map of key-value pairs representing the query parameters
     */
    std::map<std::string, std::string> parseUrlQueryParameters(const std::string &url);

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
    void sendDataToHttpsHost(const std::string &host, const std::string &path, int port, std::string &header, std::string &content, bool enable_CA = true);

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
    void run(int port, serverCallback *callback);

    /**
     * @brief Print the OpenSSL version
     */
    void print_opensslVersion();

    /**
     * @brief Print the Nghttp2 version
     */
    void print_nghttp2Version();

    /**
     * @brief Print the Zlib version
     */
    void print_zlibVersion();

    /**
     * @brief Print the cppNetworkUtil version
     */
    void print_cppNetworkUtilVersion();

private:
    std::unique_ptr<cppNetworkUtilPimpl> pimpl_; // pimpl implementation pointer
};