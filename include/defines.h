#pragma once

#include <string>

// Adjust the macro definition here
// #define IS_DEBUG // Enable debug mode
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

#define DEFAULT_HTTP_SERVER_PORT 80   // Default port for HTTP
#define DEFAULT_HTTPS_SERVER_PORT 443 // Default port for HTTPS

#define INFINITY 2147483647

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
        //		std::std::cout << "name:" << name << "  Data (partial): " << data.substr(0, std::min((size_t)50, data.length())) << (data.length() > 50 ? "..." : "") << "\n" << "filename" << filename << "content_type=" << content_type << "\n";
    }
};