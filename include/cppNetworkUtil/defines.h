#pragma once

#include <regex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#endif

// Adjust the macro definition here
// #define IS_DEBUG // Enable debug mode
#define BUFFERSIZE 4096
#define NAME "cppNetworkUtil"

// Do not modify the following macros unless you know what you are doing
#define PROTOCOL "HTTP/1.1"
#define VERSION "1.0.0"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#define DEFAULT_HTTP_SERVER_PORT 80                     // Default port for HTTP
#define DEFAULT_HTTPS_SERVER_PORT 443                   // Default port for HTTPS
#define BEHAVIOR_MODE_REDIRECT_HTTP_REQUEST_TO_HTTPS 0  // REDIRECT HTTP REQUEST TO HTTPS
#define BEHAVIOR_MODE_HANDLING_HTTP_AND_HTTPS_REQUEST 1 // HANDLING HTTP AND HTTPS REQUEST
#define IP_PROTOCOL_MODE_IPV4_ONLY 0                    // ENABLE IP PROTOCOL IPV4 ONLY
#define IP_PROTOCOL_MODE_IPV6_ONLY 1                    // ENABLE IP PROTOCOL IPV6 ONLY
#define IP_PROTOCOL_MODE_IPV4_AND_IPV6_BOTH 2           // ENABLE IP PROTOCOL IPV4 AND IPV6 BOTH
#define DEFAULT_CERT_PATH "server.crt"                  // Default public key path
#define DEFAULT_KEY_PATH "server.key"                   // Default private key path
#define DEFAULT_PRINT_LISTEN_INFO true                  // Default print listen info
#define DISABLE_HTTP_REQUEST -1                         // DISABLE HTTP REQUEST
#define DISABLE_HTTPS_REQUEST -1                        // DISABLE HTTPS REQUEST
#define CONTINUE_HANDLING 0                             // Continue handling (e.g., for error handling)
#define END_HANDING 1                                   // End handling, send the response immediately
#define PROCESSED_INTERNALLY 2                          // Processed internally, no further action needed
#define CONTINUE_ROUTING 3                              // Continue routing to next matching route

#define INFINITY 2147483647

struct requestContext
{
    SOCKET client_socket;
    bool is_https_connection;
    std::string ip;
    int port;
    std::string family;
    std::string request_data;
    std::string request_headers;
    std::string request_content;
    std::unordered_map<std::string, std::string> path_params;
    std::unordered_map<std::string, std::string> query_params;
    std::unordered_map<std::string, std::string> parsed_request_headers;
};

struct responseContext
{
    int status_code;
    std::unordered_map<std::string, std::string> response_headers;
    std::string response_content;
};

using routeHandler =
    std::function<int(const requestContext &, responseContext &)>; // 路由处理函数类型：接收请求和响应的引用

// regex 标志，包含优化选项
const std::regex::flag_type REGEX_FLAGS = std::regex::ECMAScript | std::regex::icase | std::regex::optimize;

// 路由信息结构体
struct routeInfo
{
    std::regex method_regex;
    std::regex path_regex;
    std::vector<std::string> param_names;
    routeHandler handler;
    std::string path_pattern;
    int status_code; // 用于存储状态码路由
};

// 用于存储解析过的post的multipart数据
struct multipartData
{
    std::string data;
    std::string filename;
    std::string content_type;

    multipartData() : data(""), filename(""), content_type("")
    {
    }

    void print() const
    {
        if (!filename.empty())
        {
            printf("Filename: %s\n", filename.data());
        }
        if (!content_type.empty())
        {
            printf("Content-Type: %s\n", content_type.data());
        }
        if (!data.empty())
        {
            printf("Data: %s\n", data.data());
        }
        printf("\n");
        //		std::std::cout << "name:" << name << "  Data (partial): " << data.substr(0, std::min((size_t)50,
        // data.length())) << (data.length() > 50 ? "..." : "") << "\n" << "filename" << filename << "content_type=" <<
        // content_type << "\n";
    }
};