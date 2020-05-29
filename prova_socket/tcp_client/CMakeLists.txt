cmake_minimum_required(VERSION 3.15)
project(tcp_client)

set(CMAKE_CXX_STANDARD 17)


add_executable(tcp_client main.cpp)
target_link_libraries(tcp_client ws2_32)