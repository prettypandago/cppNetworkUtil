#pragma once

// Adjust the macro definition here
#define IS_DEBUG // Enable debug mode
// #define DISABLE_HTTPS                 // Disable HTTPS support
// #define DISABLE_PRINT_LISTEN_INFO     // Disable printing listen info
#define PUBLIC_KEY_PATH "server.crt"  // Default public key path
#define PRIVATE_KEY_PATH "server.key" // Default private key path
#define BUFFERSIZE 4096
#define NAME "cppNetworkUtil"

// Do not modify the following macros unless you know what you are doing
#define PROTOCOL "HTTP/1.1"
#define VERSION "1.0.0"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#ifdef DISABLE_HTTPS
#define DEFAULT_SERVER_PORT 80 // Default port for HTTP
#else
#define DEFAULT_SERVER_PORT 443 // Default port for HTTPS
#endif

#define INFINITY 2147483647

struct responseHeaderParameters
{
    int status;                                              // status code
    std::string mime_type;                                   // mime type
    std::string content_language;                            // content language
    std::string cookie;                                      // cookie
    int Strict_Transport_Security_max_age;                   // Strict-Transport-Security header max-age
    std::string Strict_Transport_Security_includeSubDomains; // Strict-Transport-Security header
    std::string Cache_Control;                               // Cache-Control header

    responseHeaderParameters(int in_status = 200, std::string in_mime_type = "*/*", std::string in_content_language = "en-us", std::string in_cookie = "", int in_Strict_Transport_Security_max_age = 31536000, std::string in_Strict_Transport_Security_includeSubDomains = "includeSubDomains; preload", std::string in_Cache_Control = "no-cache") : status(in_status), mime_type(in_mime_type), content_language(in_content_language), cookie(in_cookie), Strict_Transport_Security_max_age(in_Strict_Transport_Security_max_age), Strict_Transport_Security_includeSubDomains(in_Strict_Transport_Security_includeSubDomains), Cache_Control(in_Cache_Control) {}
};

struct requestHeaderParameters
{
    std::string method;     // request method
    std::string connection; // connection type
    std::string host;       // host
    std::string path;       // path
    int port;               // port

    requestHeaderParameters(std::string in_method = "GET", std::string in_connection = "close", std::string in_host = "example.com", std::string in_path = "/", int in_port = 80) : method(in_method), connection(in_connection), host(in_host), path(in_path), port(in_port) {}
};

// 用于存储解析过的post的multipart数据
struct multipartData
{
    std::string data;
    std::string filename;
    std::string content_type;

    multipartData() : data(""), filename(""), content_type("") {}

    void print() const
    {
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