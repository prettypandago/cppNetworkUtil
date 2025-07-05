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

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#ifdef DISABLE_HTTPS
#define DEFAULT_SERVER_PORT 80 // Default port for HTTP
#else
#define DEFAULT_SERVER_PORT 443 // Default port for HTTPS
#endif

#define INFINITY 2147483647

// HTTP status code
#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_ACCEPTED 202
#define HTTP_NO_CONTENT 204
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302
#define HTTP_NOT_MODIFIED 304
#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_REQUEST_TIMEOUT 408
#define HTTP_CONFLICT 409
#define HTTP_GONE 410
#define HTTP_LENGTH_REQUIRED 411
#define HTTP_PAYLOAD_TOO_LARGE 413
#define HTTP_URI_TOO_LONG 414
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_TOO_MANY_REQUESTS 429
#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_BAD_GATEWAY 502
#define HTTP_SERVICE_UNAVAILABLE 503
#define HTTP_GATEWAY_TIMEOUT 504

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