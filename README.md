# cppNetworkUtil - Modern High-Performance C++ Networking Utility Library

[![许可证](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++标准](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![构建状态](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

## 🌐 Other language versions
[English](README.md "English version"), [Simplified Chinese](README_zh-cn.md "Simplified Chinese version")

## ⚠️ Warning

The project is in **development**, usage may undergo **significant changes**, and it is **not recommended for production use** yet!

## 🚀 Introduction

**cppNetworkUtil** is a lightweight, high-performance C++ networking library designed for building efficient **HTTP/HTTPS** servers and **RESTful APIs**. This project aims to provide a clean and stable interface by using the **PIMPL (Pointer to Implementation)** pattern to encapsulate complex internal implementation details and external library dependencies, greatly improving interface stability and compilation efficiency. This project also leverages **C++17** language features and proven design patterns to provide a solid foundation for C++ backend development.

## ✨ Key Features

* 🎮 **High-performance asynchronous handling**: Uses a custom thread pool (threadPool) to process client connections asynchronously, ensuring the listening thread is never blocked.

* 👍 **Pimpl pattern**: Core logic (cppNetworkUtilPimpl) uses the Pimpl (Pointer to Implementation) design pattern to separate interface and implementation, improving compile times and library stability. OpenSSL files are not required if you don't build the cppNetworkUtil library.

* ✅ **Cross-platform design**: Built with standard C++ and CMake, with networking compatibility considerations for Windows (Winsock), POSIX, and Linux (untested).

* 🔐 **HTTPS support**: Integrates the OpenSSL library for secure connections (HTTPS).

* 🎯 **Extensible routing**: Provides clear and powerful interfaces for defining HTTP routes and request handling.

## 🧰 Dependencies

1. **C++ compiler**: Supports the C++17 standard

2. **Build system**: CMake (version 3.10 or newer)

3. **SSL library**: OpenSSL, for HTTPS support **(only required when building the library)**

4. **OS-specific link libraries**:

    * Windows (MinGW): depends on *ws2_32.lib* (Winsock), *crypt32.lib*, *gdi32.lib*.

## 🤝 Contributing

Pull Requests and Issues are welcome!

## 📄 License

This project is open-source under the [MIT License](LICENSE "License").


### Thank you for using!