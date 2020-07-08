# cmake版本
cmake_minimum_required(VERSION 2.5)

# 项目名称
project(mymuduo)

# mymuduo最终编译成的so库，设置动态库的路径，放在根目录下的lib里
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息，以及启动C++11语言标准 : -std=c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -FPIC")

# 定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)

# 编译动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})
