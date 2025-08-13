# cppNetworkUtil

## Other Language Versions
[English](README.md "English version"), [简体中文](README_zh-cn.md "Simplified Chinese version")

## Warning

The project is in the **development stage**. Usage may **change significantly**, and it is **not recommended for production environments** at this time!

## Introduction

cppNetworkUtil is a simple and easy-to-use C++ network communication library supporting HTTP and HTTPS protocols.

## How to Use

1. Environment Setup

    1. It is recommended to install VSCode
    2. Configure the C++ development environment in VSCode
    3. Install the CMake plugin

2. Download

    There are two ways:
    - Download only `./build/libcppNetworkUtilLib.a`, `./include`, and `./openssl/lib` (use the precompiled static library)
    - Download all files (compile the static library yourself)

    Copy the downloaded files to your project directory.

    If you choose to compile the static library yourself, you can adjust macro definitions in `include/defines.h`.

    Then run CMake to compile.

    Write your code, [click to view example](example.cpp "Example").

    Finally, write the CMake file (Windows) and compile:

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

    After compilation, you can run the program.

3. Notes

    - If the program crashes, try running it in `cmd`. If you see `No such file or directory` and HTTPS support is not disabled, it means the certificate file is missing. You can generate it with the following commands:

        1. `openssl genrsa -out server.key 2048`
        2. `openssl req -x509 -new -nodes -key server.key -sha256 -days 365 -out server.crt`

    Normally, the program should run.

4. Successfully Running ✅✅✅

    - Congratulations! Find the generated program in your output directory (default `./`) and run it.

Thank you for using!