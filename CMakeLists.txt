
# 最终要编译成动态库
# 脱离boost库，用C++11语言本身完成
cmake_minimum_required(VERSION 2.5)
# 项目名字
project(mymuduo)

# cmake=》makefile make，如果添加了新文件，需要重新cmake产生makefile，才能让新文件参与到编译
# 需要cmake文件右键清理所有项目并重新配置，然后清理所有项目重新生成
# mymudo最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息 以及启动C++11语言标准
# -fPIC制作动态库，生成与位置无关的动态库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17 -fPIC")

# 定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
# 编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})

# 最终生成libmuduo.so



