cmake_minimum_required(VERSION 3.15)
project(tcp_server)

set(CMAKE_CXX_STANDARD 17)

add_executable(tcp_server main.cpp)
target_link_libraries(tcp_server ws2_32)