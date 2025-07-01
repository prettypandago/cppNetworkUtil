@echo off
title build_library

REM 编译 cppNetworkUtil.cpp 到对象文件
g++ -c ./src/cppNetworkUtil.cpp -o ./lib/cppNetworkUtil.o -I ./include -std=c++17

REM 编译 cppNetworkUtilImpl.cpp 到对象文件
g++ -c ./src/cppNetworkUtilImpl.cpp -o ./lib/cppNetworkUtilImpl.o -I ./include -I ./openssl/include -std=c++17

REM 编译 threadPool.cpp 到对象文件
g++ -c ./src/threadPool.cpp -o ./lib/threadPool.o -I ./include -std=c++17

REM 将你自己的对象文件打包成静态库。
ar rcs ./lib/libcppNetworkUtil.a ./lib/cppNetworkUtil.o ./lib/cppNetworkUtilImpl.o ./lib/threadPool.o

pause