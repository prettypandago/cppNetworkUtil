# cppNetworkUtil

## Other language versions
[English](README.md "English version"), [Simplified Chinese](README_zh-cn.md "Simplified Chinese version").

## Warning

This is in the **development** stage, the usage may change **significantly**, and it is **not** recommended to use it! ! !

## Introduction

Simple and easy-to-use C++ network communication library that supports HTTP and HTTPS protocols

## How to use

1. Configure the environment

1. Install vscode (recommended)

2. Configure vscode c++ development environment

3. Install cmake plugin

2. Download:

1. Download all files to a local folder and put them in a suitable folder

2. Unzip

3. Compile

1. Before compiling, you can adjust the macro definition in `include/defines.h`

```cpp
#pragma once

// Adjust the macro definition here

// defines... (Modify only these)

// Do not modify the following macros unless you know what you are doing
```

Then save it and you're done

2. Open the file `CMakeLists.txt`

3. Now you can edit the file. After editing, please proceed to the next operation

4. make (usually press Ctrl + S to save and it will automatically make)

5. build (a `Generate` button may appear in the lower left corner now, click it to build)

4. Compile your own code

1. Congratulations, the sample code has been compiled. If you need to compile other files, please continue reading, otherwise please skip

2. Now go to the output directory you set (the default is `./build`) and find the static link library you compiled (the default is `libcppNetworkUtilLib.a`). Please create a new project and copy this static link library to the `lib` folder under your new project folder (recommended)

3. By the way, you also need to copy the files under `openssl/lib` under the previous project to the `lib` folder under your new project folder (recommended)

4. After that, please copy the files under `include` under the previous project to the `include` folder under your new project folder (recommended)

5. Now, you can create a folder `src` is used to place source files, and then create a cpp file in it

6. Add include to the .cpp file:

```cpp
#include "cppNetworkUtil.h"
#include "serverCallback.h"
```

7. Add a structure to your .cpp file:

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
The `onDataReceived` function in this structure will be called after the client connects. You can write the client processing code in this function, [click to view example](example.cpp "example")

8. Then add a `main()` function:

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

9. Finally, go to the project root directory, create a `CMakeLists.txt`, and write the script in it:

```cmake
cmake_minimum_required(VERSION 3.10)
project(cppPlayer CXX) # Your project name

set(CMAKE_CXX_STANDARD 17) # Or you 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

add_executable(cppPlayer src/main.cpp) 

target_include_directories(cppPlayer PRIVATE 
${CMAKE_CURRENT_SOURCE_DIR}/include 
) 

target_link_libraries(cppPlayer PRIVATE 
${CMAKE_CURRENT_SOURCE_DIR}/lib/libcppNetworkUtilLib.a # Your static library 
${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.a # openssl libssl.a 
${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.a # openssl libcrypto.a 
# only Windows 
ws2_32 
iphlpapi 
#Mayneed 
gdi32 
crypt32 
) set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
```

10. Now `CMakeLists.txt` has been written. Please compile according to the steps mentioned above. If nothing unexpected happens, your file has been output to the project root directory.

5. Notes

- If you crash after opening, you can open a `cmd` and open this program in it. If it shows words like `No such file or directory`, if you have not disabled HTTPS support, it means that you do not have a certificate file. You can use the following command to generate it:

1. `openssl genrsa -out server.key 2048`

2. `openssl req -x509 -new -nodes -key server.key -sha256 -days 365 -out server.crt`

If nothing unexpected happens, your program should be able to run

6. Run✅✅✅

- It's done! Go to the output directory you set (default is `./build`) to find your output program and run it

😊Thank you for using it!!! 😊