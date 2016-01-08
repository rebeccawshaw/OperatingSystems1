cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -g -Werror")

add_executable(relay userspace.c)
