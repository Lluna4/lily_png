cmake_minimum_required(VERSION 3.31)
project(lily_png)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(file_reader)
target_include_directories(file_read INTERFACE "file_reader")
find_package(ZLIB)

add_library(lily_png STATIC src/lily_png.cpp
        src/metadata.cpp
        src/metadata.h
        src/utils.cpp
        src/utils.h
        src/filter.cpp
        src/filter.h
        src/convert.cpp
        src/convert.h
        src/defilter.cpp
        src/defilter.h)
target_link_libraries(lily_png PRIVATE file_read ZLIB::ZLIB)

