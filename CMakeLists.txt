cmake_minimum_required(VERSION 2.8.12.2)
project(StreamResolver)

set(CMAKE_BUILD_TYPE "Debug")

#头文件
include_directories(Include)
include_directories(Source)

#设置编译参数
set(CMAKE_CXX_FLAGS "-ffunction-sections -fdata-sections -Wall -std=c++11 -DPOSIX -DDEBUG_")

#导入库
add_library(Infra STATIC IMPORTED)
set_target_properties(Infra PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/Lib/libInfra.a)

# 设置可执行文件的输出路径(EXCUTABLE_OUTPUT_PATH是全局变量)
set(EXECUTABLE_OUTPUT_PATH ./)

#生成静态库
add_library(StreamResolver
        Src/rtp/rtp_demux.cpp
        Src/rtp/rtp_header.cpp
        Src/ps/ps_demux.cpp
        Src/h26x/H264ParseSPS.cpp
        Src/h26x/H265ParseSPS.cpp
        Src/h26x/h26x_parser.cpp
        Src/h26x/MediaBuffer.cpp)

##生成动态库
#add_library(StreamResolver_S SHARED
#        Src/rtp/rtp_demux.cpp
#        Src/rtp/rtp_header.cpp
#        Src/ps/ps_demux.cpp)
