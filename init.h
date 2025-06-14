#pragma once

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x "=" PRINT_MACRO_HELPER(x)

#define IS_DEBUG (1)
// #define IS_DEBUG (0)

#define NAME "cppNetworkUtil"
#define PROTOCOL "HTTP/1.1"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#define DEFAULT_SERVER_PORT 80;

#define INFINITY 2147483647
#define BUFFERSIZE 4096