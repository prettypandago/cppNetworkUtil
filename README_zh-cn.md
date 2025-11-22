# cppNetworkUtil - 现代高性能 C++ 网络服务工具库

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Standard](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

## 🌐 其他语言版本
[English](README.md "English version"), [简体中文](README_zh-cn.md "简体中文版")

## ⚠️ 警告

项目处于**开发阶段**，使用方式可能会有**较大变动**，**暂不推荐生产环境使用**！

## 🚀 介绍

**cppNetworkUtil** 是一个轻量级、高性能的 C++ 网络通信库，专为构建高效的 **HTTP/HTTPS** 服务器和 **RESTful API** 设计。本项目致力于提供简洁且稳定的接口，通过采用 **PIMPL (Pointer to Implementation)** 模式，成功地将复杂的底层实现细节和外部库依赖进行封装，极大地提高了接口的稳定性和编译效率。本项目致力于提供简洁且稳定的接口，通过采用 **C++17** 标准特性和成熟的设计模式，为 C++ 后端开发提供了一个强有力的基础框架。

## ✨ 核心特性

* 🎮 **高性能异步处理**：使用自定义的线程池 (threadPool) 来异步处理客户端连接，确保监听线程不会阻塞。

* 👍 **Pimpl 模式**：核心逻辑 (cppNetworkUtilPimpl) 采用 Pimpl（Pointer to Implementation）设计模式，实现接口和实现的分离，提高编译速度和库的稳定性，如果不编译 cppNetworkUtil 库就不需要 Openssl 文件。

* ✅ **跨平台设计**： 基于标准 C++ 和 CMake 构建，并为 Windows (Winsock) ， POSIX 和 Linux (未验证) 平台做了网络层兼容性考虑。

* 🔐 **HTTPS 支持**：集成 OpenSSL 库，支持安全连接 (HTTPS)。

* 🎯 **可扩展路由**：提供清晰强大的接口用于定义 HTTP 路由和请求处理。

## 🧰 依赖环境

1. **C++ 编译器**：支持 C++17 标准

2. **构建系统**：CMake (版本 3.10 或更高)

3. **SSL 库**： OpenSSL，用于 HTTPS 支持 **(仅在构建库时需要)**

4. **操作系统特定链接库**：

    * Windows (MinGW): 依赖 *ws2_32.lib* (Winsock), *crypt32.lib*, *gdi32.lib*。

## 🤝 贡献

欢迎提交 Pull Request 或 Issue！

## 📄 许可证

本项目采用 [MIT License](LICENSE "许可证") 开源授权。


### 感谢你的使用！