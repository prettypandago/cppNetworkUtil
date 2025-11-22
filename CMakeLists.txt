cmake_minimum_required(VERSION 3.10)
project(cppNetworkUtil CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 手动指定 OpenSSL 路径和库文件 **

# 1. 定义 OpenSSL 的头文件和库文件的路径
set(openssl_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/include/openssl")
set(openssl_lib_dir "${CMAKE_CURRENT_SOURCE_DIR}/lib/openssl")

# 2. 添加头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/cppNetworkUtil)
# 确保编译器能找到 OpenSSL 的头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include) 

# 3. 添加库文件搜索路径
link_directories(${openssl_lib_dir})

# 4. 手动定义 OpenSSL 的库文件列表
set(openssl_libraries 
      "${openssl_lib_dir}/libssl.a"
      "${openssl_lib_dir}/libcrypto.a"
)

# 查找线程库 (仅在 Unix-like 系统上需要)
if(UNIX)
      find_package(Threads REQUIRED)
endif()

# --- 定义并构建静态链接库 cppNetworkUtilLib ---
set(network_lib_sources
      src/cppNetworkUtil/cppNetworkUtil.cpp
      src/cppNetworkUtil/cppNetworkUtilPimpl.cpp
      src/cppNetworkUtil/threadPool.cpp
)

# 创建名为 cppNetworkUtilLib 的静态库
add_library(cppNetworkUtilLib STATIC ${network_lib_sources})

# 链接 OpenSSL 依赖的系统库
target_link_libraries(cppNetworkUtilLib PUBLIC
                                 ${openssl_libraries}
                                 $<$<BOOL:${UNIX}>:Threads::Threads>
                                 $<$<BOOL:${WIN32}>:ws2_32>
                                 $<$<BOOL:${WIN32}>:gdi32>
                                 $<$<BOOL:${WIN32}>:crypt32>
                                 )

target_link_options(cppNetworkUtilLib PUBLIC
      "-static"
      "-O3"
)

# ar 合并指令

# 定义 ar 工具和合并后的库文件名
set(ar_tool ${CMAKE_AR})
# 原始库路径和名称
set(original_lib_name "libcppNetworkUtilLib.a")
set(original_lib_path "${CMAKE_CURRENT_BINARY_DIR}/${original_lib_name}")
# 临时合并后的库名称
set(temp_merged_lib_name "libcppNetworkUtilLib_merged_temp.a")
set(temp_merged_lib_path "${CMAKE_CURRENT_BINARY_DIR}/${temp_merged_lib_name}")

# 1. 定义一个用于 ar 合并的临时脚本 (.mri)。
set(merge_script_file "${CMAKE_CURRENT_BINARY_DIR}/merge_openssl_static.mri")
file(WRITE "${merge_script_file}"
      "CREATE ${temp_merged_lib_path}\n"  # 创建临时合并文件
      "ADDLIB ${original_lib_path}\n"     # 添加原始库
      "ADDLIB ${openssl_lib_dir}/libssl.a\n"
      "ADDLIB ${openssl_lib_dir}/libcrypto.a\n"
      "SAVE\n"
      "END\n"
)

# 2. 创建一个名为 'merge_libraries' 的自定义目标来执行合并操作
add_custom_target(merge_libraries ALL
    # 合并操作：使用管道 (|) 确保脚本内容被正确传递给 ar -M
      COMMAND ${CMAKE_COMMAND} -E cat "${merge_script_file}" "|" ${ar_tool} -M
    # 删除原始库
    COMMAND ${CMAKE_COMMAND} -E remove "${original_lib_path}"
    # 将合并后的临时文件重命名为原始库的名称 (覆盖)
    COMMAND ${CMAKE_COMMAND} -E rename "${temp_merged_lib_path}" "${original_lib_path}"
    
    # 显式声明这个目标输出的文件，虽然它是对原始文件的覆盖
    BYPRODUCTS ${original_lib_path}
    COMMENT "Merging cppNetworkUtilLib and OpenSSL static libraries into ${original_lib_name}."
)

# 3. 确保合并目标在 cppNetworkUtilLib 构建之后运行
add_dependencies(merge_libraries cppNetworkUtilLib)

# --- 定义并构建一个可执行文件来测试 cppNetworkUtilLib ---
set(executable_sources
      example.cpp
)

add_executable(example ${executable_sources})

target_link_options(example PRIVATE
      "-static"
      "-O3"
)

# 1. 确保 example 只有在 merge_libraries 运行后才能开始链接（此时原始库已被合并文件覆盖）
add_dependencies(example merge_libraries) 

# 2. 链接到原始库的路径（现在它已经是合并后的文件）
target_link_libraries(example PRIVATE 
      ${original_lib_path}
      $<$<BOOL:${WIN32}>:ws2_32>
      $<$<BOOL:${WIN32}>:gdi32>
      $<$<BOOL:${WIN32}>:crypt32>
)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")