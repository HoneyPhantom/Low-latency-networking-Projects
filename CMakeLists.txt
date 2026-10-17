cmake_minimum_required(VERSION 3.15)
project(TcpEchoServer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native)
endif()

add_executable(server server.cpp)
add_executable(client client.cpp)
add_executable(nbserver non-blocking-server.cpp)
