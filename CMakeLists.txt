cmake_minimum_required(VERSION 3.3)
project(udptun)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")

include_directories(./include/)
include_directories(/home/lwj/Documents/installed/boost/include/)

link_directories("/home/lwj/Documents/installed/boost/lib")

link_libraries("glog")

add_executable(udptun_client udptun_client.cpp source/tool.cpp source/connection_manager.cpp
        source/connection.cpp source/random_generator.cpp)
