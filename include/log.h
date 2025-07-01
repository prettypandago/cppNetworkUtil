#pragma once

#include "defines.h"
#include "color.h"

#ifdef IS_DEBUG
#define log_e(fmt, ...)                                                                                             \
    do                                                                                                              \
    {                                                                                                               \
        time_t t = time(nullptr);                                                                                   \
        struct tm tm_info;                                                                                          \
        localtime_s(&tm_info, &t);                                                                                  \
        char time_buf[32];                                                                                          \
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                        \
        printf(RED "[ERROR][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define log_e(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif

#ifdef IS_DEBUG
#define log_w(fmt, ...)                                                                                                      \
    do                                                                                                                       \
    {                                                                                                                        \
        if (IS_DEBUG)                                                                                                        \
        {                                                                                                                    \
            time_t t = time(nullptr);                                                                                        \
            struct tm tm_info;                                                                                               \
            localtime_s(&tm_info, &t);                                                                                       \
            char time_buf[32];                                                                                               \
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                             \
            printf(YELLOW "[WARNING][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                                                    \
    } while (0)
#else
#define log_w(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif

#ifdef IS_DEBUG
#define log_d(fmt, ...)                                                                                                  \
    do                                                                                                                   \
    {                                                                                                                    \
        if (IS_DEBUG)                                                                                                    \
        {                                                                                                                \
            time_t t = time(nullptr);                                                                                    \
            struct tm tm_info;                                                                                           \
            localtime_s(&tm_info, &t);                                                                                   \
            char time_buf[32];                                                                                           \
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                         \
            printf(GRAY "[DEBUG][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                                                \
    } while (0)
#else
#define log_d(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif

#define log_i(fmt, ...)                                                                                              \
    do                                                                                                               \
    {                                                                                                                \
        time_t t = time(nullptr);                                                                                    \
        struct tm tm_info;                                                                                           \
        localtime_s(&tm_info, &t);                                                                                   \
        char time_buf[32];                                                                                           \
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);                                         \
        printf(WHITE "[INFO][%s][%s][%s][%d] " fmt NONE, time_buf, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

// 错误处理宏，用于打印 OpenSSL 错误并退出
#ifdef IS_DEBUG
#define HANDLE_ERROR(msg)            \
    do                               \
    {                                \
        printf(RED);                 \
        ERR_print_errors_fp(stderr); \
        printf("Error: %s\n", msg);  \
        printf(NONE);                \
    } while (0)
#else
#define HANDLE_ERROR(msg) \
    do                    \
    {                     \
    } while (0)
#endif