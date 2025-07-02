# cppNetworkUtil

## 其他语言版本
[English](README.md "English version"), [简体中文](README_zh-cn.md "简体中文版").

## 警告

正在**开发**阶段,使用方式可能发生**较大**变化,**不推荐**使用！！！

## 介绍

支持HTTP、HTTPS协议的简单易用的c++网络通信库

## 如何使用

1. 配置环境

    - 安装 vscode (推荐)

    - 配置 vscode c++ 开发环境

    - 安装 cmake 插件

2. 下载：

    - 将所有文件下载到本地，放在一个合适的文件夹下

    - 解压

3. 在您的项目中引用：

    - 创建一个.cpp文件

    - 在.cpp文件里添加include：

    ```cpp
    #include "cppNetworkUtil.h"
    #include "serverCallback.h"
    ```

4. 编写代码：

    - 在你的.cpp文件里添加一个结构体：
    ```cpp
    class MyServerHandler : public serverCallback
    {
    public:
        MyServerHandler(cppNetworkUtil &network_util) : network_util_(network_util) {}

        void onDataReceived(int client_id) override
        {
            //在这里编写代码
        }
    private:
        cppNetworkUtil &network_util_;
    };
    ```
    这个结构体里的`onDataReceived`函数会在客户端连接后调用，可以在这个函数里编对客户端处理的代码，[点击查看示例](example.cpp "示例")

5. 编译

    - 打开文件 `CMakeLists.txt`

    - 现在可以编辑文件，这里可以设置宏定义，控制程序的功能，注意：请把 `set(EXECUTABLE_SOURCES
    example.cpp
)`和 `add_executable(example ${EXECUTABLE_SOURCES})` 还有 `target_link_libraries(example cppNetworkUtilLib)` 这些地方的 `example` 换成你创建的cpp文件的名字

    - make (一般按 Ctrl + S 保存就会自动 make)

    - build (现在左下角可能会出现一个 `生成` 按钮，点击即可build)

6. 运行✅✅✅

    - 大功告成！到你设置的输出目录 (默认是 `./build` ) 里找到你输出的程序并运行即可

😊感谢你的使用！！！😊
