cmake_minimum_required(VERSION 3.10)
project(falcon)
set(CMAKE_CXX_STANDARD 11)

set(FALCON_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/")
include_directories(${FALCON_INCLUDE_DIR};${PROJECT_BINARY_DIR})

add_subdirectory(src)
enable_testing ()
add_subdirectory(test)