# cppNetworkUtil


## Other Language Versions
[English](README.md "English version"), [简体中文](README_zh-cn.md "Simplified Chinese version").

## Warning

Currently in **development** stage. The usage may change **significantly**. **Not recommended** for use!!!

## Introduction

A simple and easy-to-use C++ network communication library supporting HTTP and HTTPS protocols.

## How to Use

1. Configure the environment

    - Install VS Code (recommended)

    - Set up the VS Code C++ development environment

    - Install the CMake extension

2. Download:

    - Download all files locally and place them in a suitable folder

    - Extract the files

3. Reference in your project:

    - Create a `.cpp` file

    - Add includes in your `.cpp` file:

    ```cpp
    #include "cppNetworkUtil.h"
    #include "serverCallback.h"
    ```

4. Write code:

    - Add a struct in your `.cpp` file:
    ```cpp
    class MyServerHandler : public serverCallback
    {
    public:
        MyServerHandler(cppNetworkUtil &network_util) : network_util_(network_util) {}

        void onDataReceived(int client_id) override
        {
            // Write your code here
        }
    private:
        cppNetworkUtil &network_util_;
    };
    ```
    The `onDataReceived` function in this struct will be called when a client connects. You can write your client handling code in this function. [Click to view example](example.cpp "Example")

5. Compile

    - Open the file `CMakeLists.txt`

    - You can now edit the file. Here you can set macro definitions to control the program's features. Note: Please change `set(EXECUTABLE_SOURCES example.cpp)` and `add_executable(example ${EXECUTABLE_SOURCES})` as well as `target_link_libraries(example cppNetworkUtilLib)`—replace `example` with the name of your `.cpp` file.

    - make (usually pressing Ctrl + S to save will automatically trigger make)

    - build (a `Build` button may appear in the lower left corner, click it to build)

6. Run ✅✅✅

    - All done! Find your output program in the output directory you set (default is `./build`) and run it.

😊 Thank you for using!!! 😊