cmake_minimum_required(VERSION 3.31)
project(lily_png)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(file_reader)

add_library(lily_png STATIC lily_png.cpp)

target_link_libraries(lily_png file_read)
