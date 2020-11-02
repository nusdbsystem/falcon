cmake_minimum_required(VERSION 3.10)
project(falcon_test)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

#set(FALCON_PROJECT_DIR "/home/wuyuncheng/Documents/projects/falcon/")
#include_directories(${FALCON_PROJECT_DIR})
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

#link_directories(/usr/ssl/lib)

include(FindProtobuf)
find_package(Protobuf REQUIRED)

include_directories(
        $ENV{HOME}/Documents/projects/boost_1_64_0/stage/lib
        ${PROJECT_SOURCE_DIR}/include/
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        ${PROJECT_SOURCE_DIR}/src)

set(TEST_SOURCE_FILES test_main.cc
        falcon/test_fixed_point.cc
        falcon/test_pb_converter.cc
        falcon/test_djcs_t_aux.cc)

add_executable(falcon_test ${TEST_SOURCE_FILES})

target_link_libraries(falcon_test
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
        ${PROTOBUF_LIBRARY}
        executor
        libboost_thread.a
        libboost_system.a
        pthread
        crypto
        cryptopp
        gtest
        gflags
        ssl
        mpirxx
        mpir
        sodium
        /usr/local/lib/libglog.so
        /usr/lib/x86_64-linux-gnu/libgmp.so
        /usr/lib/x86_64-linux-gnu/libgmpxx.so)
