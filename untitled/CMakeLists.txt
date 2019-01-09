cmake_minimum_required(VERSION 3.12)
project(untitled C)

set(CMAKE_C_STANDARD 99)

add_executable(untitled main.c threadpool.h threadpool.cpp)

find_package(Threads REQUIRED)
target_link_libraries(untitled Threads::Threads)