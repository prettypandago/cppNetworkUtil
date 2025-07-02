# # CMakeLists.txt
# cmake_minimum_required(VERSION 3.10)
# project(cppNetworkUtilLib CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openssl")

# # 添加头文件搜索路径
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/include)

# # 添加库文件搜索路径
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib)

# # 查找所需的库
# find_package(OpenSSL REQUIRED)
# # if(UNIX)
# #     find_package(Threads REQUIRED)
# # endif()

# # --- 核心修改：添加静态链接库 ---
# # 定义组成静态库的源文件
# set(NETWORK_LIB_SOURCES
#     src/cppNetworkUtil.cpp
#     src/cppNetworkUtilPimpl.cpp
#     src/threadPool.cpp
# )

# # 使用 add_library 命令创建静态库
# # 'NetworkLib' 是你给这个库起的名字，它会生成 libNetworkLib.a (Linux/macOS) 或 NetworkLib.lib (Windows)
# # STATIC 关键字明确指示 CMake 构建一个静态库
# add_library(cppNetworkUtilLib STATIC ${NETWORK_LIB_SOURCES})

# target_compile_definitions(cppNetworkUtilLib
#     PUBLIC
#         # IS_DEBUG
#         # DISABLE_HTTPS
#         # DISABLE_PRINT_LISTEN_INFO
#         PUBLIC_KET_PATH="../server.crt"  # Public key path
#         PRIVATE_KEY_PATH="../server.key"  # Private key path
# )

# # 链接库的依赖
# # NetworkLib 本身也可能依赖其他库（如 OpenSSL、线程库）
# # PUBLIC 关键字表示这些依赖也会传递给所有链接 NetworkLib 的目标 (比如 MyServerProgram)
# target_link_libraries(cppNetworkUtilLib PUBLIC
#                       ${OpenSSL_LIBRARIES}
#                       ws2_32           # Windows 系统使用
#                       ssl
#                       crypto
#                       ws2_32
#                       gdi32
#                       crypt32
#                      )

# # --- 添加可执行文件 ---
# set(EXECUTABLE_SOURCES
#     example.cpp
#     # 如果 MyServerHandler.cpp 包含在 main.cpp 中，或者它本身不生成可执行文件，则不需要在这里列出
#     # 如果 MyServerHandler.cpp 是一个独立的源文件，且其代码没有被库包含，则需要
#     # src/MyServerHandler.cpp
# )

# # 添加一个名为 example 的可执行文件
# add_executable(example ${EXECUTABLE_SOURCES})

# # 链接可执行文件到你刚刚构建的静态库
# # 这样做会把 NetworkLib 的代码嵌入到 MyServerProgram 中
# target_link_libraries(example cppNetworkUtilLib)

# cppNetworkUtil 项目的 CMakeLists.txt
# cppNetworkUtil/CMakeLists.txt

# cmake_minimum_required(VERSION 3.10)
# project(cppNetworkUtil CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openssl")

# # 添加头文件搜索路径
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/include)

# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib)

# # 定义编译宏 (例如用于调试)
# add_compile_definitions(IS_DEBUG)

# # 查找 OpenSSL 库
# # 仍然需要，因为我们要获取 OpenSSL 静态库的路径来合并。
# # 确保你的 OpenSSL 是静态编译的，并且 find_package 能找到它的静态库。
# find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

# # 查找线程库 (仅在 Unix-like 系统上需要)
# if(UNIX)
#     find_package(Threads REQUIRED)
# endif()

# # --- 核心：构建 cppNetworkUtilLib.a 并合并 OpenSSL ---

# # 定义组成静态库的源文件
# set(NETWORK_LIB_SOURCES
#     src/cppNetworkUtil.cpp
#     src/cppNetworkUtilPimpl.cpp
#     src/threadPool.cpp
# )

# # 1. 编译你的源文件到对象文件
# # add_library(cppNetworkUtilLib_Objects OBJECT ${NETWORK_LIB_SOURCES})
# # (注意：为了简化 ar 命令，我们暂时不使用 OBJECT 库，直接用 add_library(STATIC) 并在 POST_BUILD 后处理)

# # 创建空的静态库 cppNetworkUtilLib
# # 链接它自己的代码 (即 NETWORK_LIB_SOURCES 编译成的对象文件)
# add_library(cppNetworkUtilLib STATIC ${NETWORK_LIB_SOURCES})

# # 链接 cppNetworkUtilLib 内部的依赖 (除了 OpenSSL，因为 OpenSSL 要被 ar 合并)
# target_link_libraries(cppNetworkUtilLib PUBLIC
#                       OpenSSL::SSL
#                       OpenSSL::Crypto
#                       $<$<BOOL:${UNIX}>:Threads::Threads> # Unix 系统 (Linux/macOS)
#                       $<$<BOOL:${WIN32}>:ws2_32>           # Windows 系统
#                       # 移除之前可能多余的 ssl 和 crypto，因为 OpenSSL::SSL 和 OpenSSL::Crypto 已经包含了它们
#                       # 移除 gdi32 和 crypt32，除非 cppNetworkUtilLib 确实直接使用了它们，
#                       # 否则通常由 OpenSSL 内部处理或由最终可执行文件链接。
#                      )

# # 2. 创建一个用于 ar 命令的指令文件 (MRI 格式)
# # 这个文件会告诉 ar 工具如何合并库
# # IMPORTANT: 使用 $<TARGET_FILE:cppNetworkUtilLib> 获取完整的库路径，更健壮。
# # 路径的斜杠方向在 Windows CMD 和 Linux Bash 中可能不同，CMake 会处理。
# file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/merge_openssl_static.mri"
# "CREATE $<TARGET_FILE:cppNetworkUtilLib>
# ADDLIB ${OPENSSL_CRYPTO_LIBRARY}
# ADDLIB ${OPENSSL_SSL_LIBRARY}
# SAVE
# END
# ")

# # 3. 添加一个构建后命令，在 cppNetworkUtilLib.a 生成后，执行 ar 合并
# add_custom_command(TARGET cppNetworkUtilLib POST_BUILD
#     COMMAND "${CMAKE_AR}" -M < "${CMAKE_CURRENT_BINARY_DIR}/merge_openssl_static.mri"
#     # 明确设置工作目录到 build 目录，以防 ar 在不正确的位置寻找临时文件
#     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
#     COMMENT "Merging OpenSSL static libraries into cppNetworkUtilLib.a"
# )


# # --- (可选) 定义并构建一个可执行文件来测试 cppNetworkUtilLib ---
# # 确保这个 example.cpp 存在于你的项目根目录
# set(EXECUTABLE_SOURCES
#     example.cpp
# )

# add_executable(example ${EXECUTABLE_SOURCES})

# # 链接可执行文件到你刚刚构建的静态库
# # 注意：由于 cppNetworkUtilLib 已经包含了 OpenSSL，这里不再需要链接 OpenSSL
# target_link_libraries(example PRIVATE cppNetworkUtilLib)

# cppNetworkUtil/CMakeLists.txt

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

# 链接可执行文件到你刚刚构建的静态库
# 由于 cppNetworkUtilLib 使用 PUBLIC 链接了所有依赖，这里的 example 目标会自动继承。
target_link_libraries(example PRIVATE cppNetworkUtilLib)