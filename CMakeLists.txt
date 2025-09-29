cmake_minimum_required(VERSION 3.10)
project(cppNetworkUtil CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 如果你将 OpenSSL 放在项目内部，并希望通过相对路径找到它
# 请确认这个路径是正确的，并且实际存在 OpenSSL 的头文件和库文件
# 如果 OpenSSL 是通过 MinGW 的 pacman 安装的，通常不需要设置此项，CMake 会自动找到。
# 如果你手动下载并解压了 OpenSSL，则需要指向其根目录。
set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openssl") # 例如：D:/YYProject/vscode-project/cppNetworkUtil/openssl

# 添加头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/cppNetworkUtil)
# 确保编译器能找到 OpenSSL 的头文件
include_directories(${OPENSSL_ROOT_DIR}/include)

# (可选) 添加库文件搜索路径，find_package 通常会处理，但明确指定也无妨
link_directories(${OPENSSL_ROOT_DIR}/lib)

# 查找 OpenSSL 库
# 这对构建 cppNetworkUtilLib 很关键，因为它内部使用了 OpenSSL。
# REQUIRED 表示如果找不到就报错。COMPONENTS SSL Crypto 明确指出需要这两个核心部分。
# 确保你的 OpenSSL 是静态编译的，find_package 能找到 .a 文件。
find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

# 查找线程库 (仅在 Unix-like 系统上需要)
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

# ！！！！！ 核心修改：在这里链接所有需要的库 ！！！！！
# PUBLIC 关键字表示链接此库的目标也会继承这些依赖。
# 这会确保 OpenSSL 的代码在构建 cppNetworkUtilLib.a 时就被包含进来。
target_link_libraries(cppNetworkUtilLib PUBLIC
                      OpenSSL::SSL
                      OpenSSL::Crypto
                      $<$<BOOL:${UNIX}>:Threads::Threads> # Unix 系统 (Linux/macOS)
                      $<$<BOOL:${WIN32}>:ws2_32>           # Windows Winsock 库
                      $<$<BOOL:${WIN32}>:gdi32>            # Windows GDI32 库 (OpenSSL 可能间接依赖)
                      $<$<BOOL:${WIN32}>:crypt32>          # Windows CryptoAPI 库 (OpenSSL 依赖)
                     )

# --- 移除所有关于 ar 合并的指令 ---
# file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/merge_openssl_static.mri" ...)
# add_custom_command(TARGET cppNetworkUtilLib POST_BUILD ...)

# --- (可选) 定义并构建一个可执行文件来测试 cppNetworkUtilLib ---
# 确保这个 example.cpp 存在于你的项目根目录
set(EXECUTABLE_SOURCES
    example.cpp
)

# 创建名为 example 的可执行文件
add_executable(example ${EXECUTABLE_SOURCES})

# 为可执行文件添加链接选项
target_link_options(example PRIVATE
    "-static"
)

# 链接可执行文件到你刚刚构建的静态库
# 由于 cppNetworkUtilLib 使用 PUBLIC 链接了所有依赖，这里的 example 目标会自动继承。
target_link_libraries(example PRIVATE cppNetworkUtilLib)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")