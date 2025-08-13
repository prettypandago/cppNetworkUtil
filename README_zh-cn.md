# cppNetworkUtil

## 其他语言版本
[English](README.md "English version"), [简体中文](README_zh-cn.md "简体中文版")

## 警告

项目处于**开发阶段**，使用方式可能会有**较大变动**，**暂不推荐生产环境使用**！

## 介绍

cppNetworkUtil 是一个支持 HTTP 和 HTTPS 协议的简单易用的 C++ 网络通信库。

## 如何使用

1. 环境配置

    1. 推荐安装 VSCode
    2. 配置 VSCode 的 C++ 开发环境
    3. 安装 CMake 插件

2. 下载

    有两种方式：
    - 仅下载 `./build/libcppNetworkUtilLib.a`、`./include` 和 `./openssl/lib`（使用已编译好的静态库）
    - 下载全部文件（自行编译静态库）

    将下载的文件复制到你的项目目录下。

    如果选择自行编译静态库，可在 `include/defines.h` 中调整宏定义

    然后运行 CMake 进行编译

    编写代码，[点击查看示例](example.cpp "示例")

    最后编写 CMake 文件（Windows），并编译：

    ```cmake
    cmake_minimum_required(VERSION 3.10)
    project(yourProjectName C CXX)
    
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    add_executable(yourProjectName src/main.cpp src/fileUtil.cpp)
    set_source_files_properties(src/sqlite3.c PROPERTIES LANGUAGE C)
    target_sources(yourProjectName PRIVATE src/sqlite3.c)

    target_include_directories(yourProjectName PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(yourProjectName PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libcppNetworkUtilLib.a
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.a
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.a 
        ws2_32
        iphlpapi
        gdi32
        crypt32
    )

    set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    ```

    编译完成后即可运行

3. 注意事项

    - 如果程序闪退，可在 `cmd` 中运行，若提示 `No such file or directory`，且未禁用 HTTPS 支持，说明缺少证书文件。可用以下命令生成：

        1. `openssl genrsa -out server.key 2048`
        2. `openssl req -x509 -new -nodes -key server.key -sha256 -days 365 -out server.crt`

    正常情况下，程序即可运行

4. 成功运行 ✅✅✅

    - 恭喜！在你设置的输出目录（默认 `./`）找到生成的程序并运行即可

感谢你的使用！ 