@echo off
title build_example

REM 编译 example.cpp 并链接你的库、OpenSSL 库、Windows 网络库 (Winsock) 和 Windows 加密库 (Crypt32)
g++ example.cpp -o example.exe ^
    -I ./include ^
    -L ./lib -lcppNetworkUtil ^
    -L ./openssl/lib -lssl -lcrypto ^
    -lws2_32 -lgdi32 -lcrypt32 ^
    -std=c++17

pause