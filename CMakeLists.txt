# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(cppNetworkUtil CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openssl")

# 添加头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/include)

# 添加库文件搜索路径
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib)

# 定义宏
add_compile_definitions(IS_DEBUG)

# 查找所需的库
find_package(OpenSSL REQUIRED)
# if(UNIX)
#     find_package(Threads REQUIRED)
# endif()

# --- 核心修改：添加静态链接库 ---
# 定义组成静态库的源文件
set(NETWORK_LIB_SOURCES
    src/cppNetworkUtil.cpp
    src/cppNetworkUtilPimpl.cpp
    src/threadPool.cpp
)

# 使用 add_library 命令创建静态库
# 'NetworkLib' 是你给这个库起的名字，它会生成 libNetworkLib.a (Linux/macOS) 或 NetworkLib.lib (Windows)
# STATIC 关键字明确指示 CMake 构建一个静态库
add_library(cppNetworkUtilLib STATIC ${NETWORK_LIB_SOURCES})

# 链接库的依赖
# NetworkLib 本身也可能依赖其他库（如 OpenSSL、线程库）
# PUBLIC 关键字表示这些依赖也会传递给所有链接 NetworkLib 的目标 (比如 MyServerProgram)
target_link_libraries(cppNetworkUtilLib PUBLIC
                      ${OpenSSL_LIBRARIES}
                    #   Threads::Threads # Unix 系统使用
                      ws2_32           # Windows 系统使用
                      ssl
                      crypto
                      ws2_32
                      gdi32
                      crypt32
                     )

# # --- 添加可执行文件 ---
# # 定义可执行文件的源文件 (现在只包含 main.cpp，因为 cppNetworkUtil 的功能封装在库里了)
# set(EXECUTABLE_SOURCES
#     example.cpp
#     # 如果 MyServerHandler.cpp 包含在 main.cpp 中，或者它本身不生成可执行文件，则不需要在这里列出
#     # 如果 MyServerHandler.cpp 是一个独立的源文件，且其代码没有被库包含，则需要
#     # src/MyServerHandler.cpp
# )

# # 添加一个名为 MyServerProgram 的可执行文件
# add_executable(example ${EXECUTABLE_SOURCES})

# # 链接可执行文件到你刚刚构建的静态库
# # 这样做会把 NetworkLib 的代码嵌入到 MyServerProgram 中
# target_link_libraries(example cppNetworkUtilLib)