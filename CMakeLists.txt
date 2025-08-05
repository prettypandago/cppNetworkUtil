cmake_minimum_required(VERSION 3.10)
project(cppNetworkUtil CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 定义第三方库的路径和名称 ---
# 请确保将 nghttp2, OpenSSL, Zlib 的编译好的文件复制到此项目下的各自名称的目录中
# 目录结构示例:
# your_project/
# ├── nghttp2/
# │   ├── include/    (nghttp2.h, nghttp2ver.h 直接位于此目录下)
# │   └── lib/        (libnghttp2.a, libnghttp2.dll.a)
# ├── openssl/
# │   ├── include/    (openssl/*.h)
# │   └── lib/        (libssl.a, libcrypto.a, libssl-3.dll.a, libcrypto-3.dll.a, etc.)
# └── zlib/
#     ├── include/    (zlib.h)
#     └── lib/        (libz.a, libz.dll.a)

# nghttp2 库路径
set(NGHTTP2_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nghttp2)
set(NGHTTP2_INCLUDE_DIRS ${NGHTTP2_BASE_DIR}/include)
# 明确指定静态库文件，如果存在且是纯静态的
set(NGHTTP2_STATIC_LIB ${NGHTTP2_BASE_DIR}/lib/libnghttp2.a)

# OpenSSL 库路径
set(OPENSSL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openssl)
set(OPENSSL_INCLUDE_DIRS ${OPENSSL_BASE_DIR}/include)
# 明确指定静态库文件
set(OPENSSL_STATIC_LIBS
    ${OPENSSL_BASE_DIR}/lib/libssl.a
    ${OPENSSL_BASE_DIR}/lib/libcrypto.a
)

# Zlib 库路径
set(ZLIB_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
set(ZLIB_INCLUDE_DIRS ${ZLIB_BASE_DIR}/include)
# 明确指定静态库文件
set(ZLIB_STATIC_LIB ${ZLIB_BASE_DIR}/lib/libz.a)

# --- 添加库文件搜索路径 ---
# 如果使用完整的库文件路径，则不需要 link_directories
# link_directories(
#     ${NGHTTP2_BASE_DIR}/lib
#     ${OPENSSL_BASE_DIR}/lib
#     ${ZLIB_BASE_DIR}/lib
# )

# --- 查找线程库 (Unix-like 系统，如 Linux/macOS) ---
if(UNIX)
    find_package(Threads REQUIRED)
endif()

# --- 定义并构建静态链接库 cppNetworkUtilLib ---

# 定义组成静态库的源文件
set(NETWORK_LIB_SOURCES
    src/cppNetworkUtil.cpp
    src/cppNetworkUtilPimpl.cpp
    src/threadPool.cpp
)

# 创建名为 cppNetworkUtilLib 的静态库
add_library(cppNetworkUtilLib STATIC ${NETWORK_LIB_SOURCES})

# >>>>>>>>>>>>> 关键修改在这里 <<<<<<<<<<<<<
# 告诉 nghttp2 的头文件我们正在链接静态库
target_compile_definitions(cppNetworkUtilLib PRIVATE NGHTTP2_STATICLIB)
# <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# 添加头文件搜索路径
target_include_directories(cppNetworkUtilLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include # 你的项目公共头文件目录
    ${NGHTTP2_INCLUDE_DIRS}             # nghttp2 的头文件路径
    ${OPENSSL_INCLUDE_DIRS}             # OpenSSL 的头文件路径
    ${ZLIB_INCLUDE_DIRS}                # Zlib 的头文件路径
)

# ！！！！！ 关键修改：调整链接顺序并明确指定静态库文件 ！！！！！
# 当一个静态库 (cppNetworkUtilLib) 内部使用了另一个静态库 (nghttp2, OpenSSL, Zlib)
# 并且最终的可执行文件 (example) 链接这个静态库时，
# 依赖的静态库需要在被依赖的静态库之后列出。
# 此外，nghttp2 依赖 OpenSSL 和 Zlib，所以它们应该在 nghttp2 之后。
target_link_libraries(cppNetworkUtilLib PUBLIC
    # 你的库本身
    # cppNetworkUtilLib 依赖 nghttp2, OpenSSL, Zlib
    # 它们应该在 cppNetworkUtilLib 的 PRIVATE 链接中，
    # 或者如果 cppNetworkUtilLib 是一个静态库，并且希望这些依赖也被打包进去，
    # 那么 PUBLIC 链接这些依赖是正确的，但顺序很重要。

    # nghttp2 库
    ${NGHTTP2_STATIC_LIB}               

    # nghttp2 依赖 OpenSSL 和 Zlib，所以它们应该在 nghttp2 之后
    ${OPENSSL_STATIC_LIBS}              # OpenSSL 库 (ssl 和 crypto)
    ${ZLIB_STATIC_LIB}                  # Zlib 库

    # 系统库
    $<$<BOOL:${UNIX}>:Threads::Threads> # Unix 系统 (Linux/macOS)
    $<$<BOOL:${WIN32}>:ws2_32>           # Windows Winsock 库
    $<$<BOOL:${WIN32}>:gdi32>            # Windows GDI32 库 (OpenSSL 可能间接依赖)
    $<$<BOOL:${WIN32}>:crypt32>          # Windows CryptoAPI 库 (OpenSSL 依赖)
)

# --- (可选) 定义并构建一个可执行文件来测试 cppNetworkUtilLib ---
set(EXECUTABLE_SOURCES
    example.cpp
)

# 创建名为 example 的可执行文件
add_executable(example ${EXECUTABLE_SOURCES})

# 链接可执行文件到构建的静态库
target_link_libraries(example PRIVATE cppNetworkUtilLib # 你的核心库
)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")