# cppNetworkUtil

## 其他语言版本
[English](README.md "English version"), [简体中文](README_zh-cn.md "简体中文版").

## 警告

正在**开发**阶段,使用方式可能发生**较大**变化,**不推荐**使用！！！

## 介绍

通过我的网络库，你可以非常容易的实现网络通信功能。

支持http和https

## 如何使用

1. 下载
2. 在您的项目中引用
3. 配置openssl环境
4. 编写代码
5. 运行✅✅✅

[点击查看示例](example.cpp "示例")

## 如何编译

`g++ example.cpp -o example.exe lib/cppNetworkUtil.a -I include/ -I openssl/include -L openssl/lib -lssl -lcrypto -lws2_32 -lcrypt32 -static-libgcc -static-libstdc++`

😊感谢你的使用！！！😊
