# cppNetworkUtil

## 其他语言版本
[English](README.md "English version"), [简体中文](README_zh-cn.md "简体中文版").

## 警告

正在**开发**阶段,使用方式可能发生**较大**变化,**不推荐**使用！！！

## 介绍

支持HTTP、HTTPS协议的简单易用的c++网络通信库

## 如何使用

1. 配置环境

    1. 安装 vscode (推荐)

    2. 配置 vscode c++ 开发环境

    3. 安装 cmake 插件

2. 下载：

    1. 将所有文件下载到本地，放在一个合适的文件夹下

    2. 解压

3. 编译

    1. 在编译前，你可以到 `include/defines.h` 里调整宏定义

    ```cpp
    #pragma once

    // Adjust the macro definition here

    // defines... (Modify only these)

    // Do not modify the following macros unless you know what you are doing
    ```

    然后保存就行了

    2. 打开文件 `CMakeLists.txt`

    3. 现在可以编辑文件，在编辑完成后，请继续执行下一个操作

    4. make (一般按 Ctrl + S 保存就会自动 make)

    5. build (现在左下角可能会出现一个 `生成` 按钮，点击即可build)

4. 编译你自己的代码

    1. 恭喜你，示例代码已经编译完成了，如果你需要编译其他文件，请继续看下去，否则请跳过

    2. 现在来到你设置的输出目录 (默认是 `./build` ) 里找到你编译的静态链接库(默认是 `libcppNetworkUtilLib.a` )，请创建一个新项目，并把这个静态链接库复制到你新项目文件夹下的 `lib` 文件夹下 (推荐) 

    3. 对了，你还需要把之前项目下的 `openssl/lib` 下的文件也复制到你新项目文件夹下的 `lib` 文件夹下 (推荐) 

    4. 之后，请你把之前项目下的 `include` 下的文件也复制到你新项目文件夹下的 `include` 文件夹下 (推荐) 

    5. 现在，你可以创建一个文件夹 `src`，用于放置源文件，然后在里面创建一个cpp文件

    6. 在.cpp文件里添加include：

    ```cpp
    #include "cppNetworkUtil.h"
    #include "serverCallback.h"
    ```

    7. 在你的.cpp文件里添加一个结构体：

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
    这个结构体里的`onDataReceived`函数会在客户端连接后调用，可以在这个函数里编对客户端处理的代码，[点击查看示例](example.cpp "示例")

    8. 之后添加一个 `main()` 函数：

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

    9. 最后来到项目根目录，创建一个 `CMakeLists.txt`，在里面编写脚本：

    ```cmake
    cmake_minimum_required(VERSION 3.10)
    project(cppPlayer CXX) # Your project name

    set(CMAKE_CXX_STANDARD 17) # 或者你
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    add_executable(cppPlayer src/main.cpp)

    target_include_directories(cppPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(cppPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcppNetworkUtilLib.a # Your static library
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.a               # openssl libssl.a
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.a             # openssl libcrypto.a
        # only Windows
        ws2_32                                                 
        iphlpapi                                               
        # May need
        gdi32                                              
        crypt32                                                
    )

    set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    ```

    10. 现在 `CMakeLists.txt` 已经编写好了，请按照之前说过的步骤编译，不出意外，你的文件已经输出到项目根目录底下了

5. 注意事项

    - 如果你打开后闪退了，可以打开一个 `cmd`，在里面打开这个程序，如果显示 `No such file or directory` 之类的字样，你如果没有禁用 HTTPS 支持，那么说明你没有证书文件，你可以使用以下命令生成：

        1. `openssl genrsa -out server.key 2048`

        2. `openssl req -x509 -new -nodes -key server.key -sha256 -days 365 -out server.crt`

    不出意外，你的程序应该已经可以运行了

6. 运行✅✅✅

    - 大功告成！到你设置的输出目录 (默认是 `./build` ) 里找到你输出的程序并运行即可

😊感谢你的使用！！！😊
