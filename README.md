# cppNetworkUtil

## Other Language Versions
[English](README.md "English version"), [简体中文](README_zh-cn.md "Simplified Chinese version").

## Warning

This project is in the **development** stage. The usage may change **significantly**. **Not recommended** for production use!

## Introduction

A simple and easy-to-use C++ network communication library supporting HTTP and HTTPS protocols.

## How to Use

1. Environment Setup

    1. Install VS Code (recommended)

    2. Set up the VS Code C++ development environment

    3. Install the CMake extension

2. Download:

    1. Download all files to a local folder

    2. Extract the files

3. Build

    1. Before building, you can adjust macro definitions in `include/defines.h`

    ```cpp
    #pragma once

    // Adjust the macro definition here

    // defines... (Modify only these)

    // Do not modify the following macros unless you know what you are doing
    ```

    Save the file after making changes.

    2. Open `CMakeLists.txt`

    3. Edit files as needed, then proceed to the next step

    4. Build (usually, pressing Ctrl + S will trigger an automatic build)

    5. Build (a `Build` button may appear in the lower left corner, click it to build)

4. Build Your Own Code

    1. Congratulations, the sample code has been built. If you want to build other files, continue reading; otherwise, you can skip this section.

    2. Go to your output directory (default is `./build`) and find the static library you built (default is `libcppNetworkUtilLib.a`). Create a new project and copy this static library to the `lib` folder in your new project directory (recommended).

    3. Also, copy the files from the previous project's `openssl/lib` folder to the `lib` folder in your new project directory (recommended).

    4. Next, copy the files from the previous project's `include` folder to the `include` folder in your new project directory (recommended).

    5. Now, create a `src` folder for your source files, and add a `.cpp` file inside.

    6. In your `.cpp` file, add the following includes:

    ```cpp
    #include "cppNetworkUtil.h"
    #include "serverCallback.h"
    ```

    7. Add a handler class in your `.cpp` file:

    ```cpp
    class MyServerHandler : public serverCallback
    {
    public:
        MyServerHandler(cppNetworkUtil &network_util) : network_util_(network_util) {}

        void onDataReceived(int client_id) override
        {
            // code
        }
    private:
        cppNetworkUtil &network_util_;
    };
    ```
    The `onDataReceived` function will be called when a client connects. You can handle client logic here. [See example](example.cpp "Example")

    8. Add a `main()` function:

    ```cpp
    int main(int argc, char **argv)
    {
        cppNetworkUtil server;
        MyServerHandler handler(server);

        try
        {
            server.run(DEFAULT_SERVER_PORT, &handler);
        }
        catch (const std::exception &e)
        {
            std::cout << "Error: " << e.what() << "\n";
        }
        catch (...)
        {
            std::cout << "Unknown error occurred.\n";
        }

        return 0;
    }
    ```

    9. In your project root, create a `CMakeLists.txt` file with the following content:

    ```cmake
    cmake_minimum_required(VERSION 3.10)
    project(cppPlayer CXX) # Your project name

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    add_executable(cppPlayer src/main.cpp)

    target_include_directories(cppPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(cppPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcppNetworkUtilLib.a # Your static library
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.a               # openssl libssl.a
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.a            # openssl libcrypto.a
        # only Windows
        ws2_32
        iphlpapi
        # May need
        gdi32
        crypt32
    )

    set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    ```

    10. Now your `CMakeLists.txt` is ready. Follow the previous steps to build. If everything goes well, your executable will be in the project root directory.

5. Notes

    - If the program crashes on launch, open a `cmd` window and run the program there. If you see an error like `No such file or directory` and you haven't disabled HTTPS support, it means you don't have the certificate files. You can generate them with:

        1. `openssl genrsa -out server.key 2048`

        2. `openssl req -x509 -new -nodes -key server.key -sha256 -days 365 -out server.crt`

    If everything is set up correctly, your program should run.

6. Run ✅✅✅

    - All done! Go to your output directory (default is `./build`), find your executable, and run it.

😊 Thank you for using cppNetworkUtil! 😊

