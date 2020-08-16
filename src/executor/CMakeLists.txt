cmake_minimum_required(VERSION 3.10)
project(falcon)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

set(SOURCE_FILES main.cc
        network/Comm.hpp
        network/Comm.cpp
        network/ConfigFile.hpp
        network/ConfigFile.cpp
        party/party.h
        party/party.cc
        utils/pb_converter/model_pb_converter.h
        utils/pb_converter/model_pb_converter.cc
        message/model.pb.h
        message/model.pb.cc
        operator/phe/fixed_point_encoder.h
        operator/phe/fixed_point_encoder.cc)

set(SPDZ_HOME "/home/wuyuncheng/Documents/projects/Pivot-SPDZ/")

include(FindProtobuf)
find_package(Protobuf REQUIRED)

include_directories($ENV{HOME}/Documents/projects/boost_1_64_0/stage/lib
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        /usr/local/lib)

#find_package (glog 0.4.0 REQUIRED)

link_directories(/usr/ssl/lib ${SPDZ_HOME}/local/lib)

add_executable(falcon ${SOURCE_FILES})

target_link_libraries(falcon
        libboost_thread.a libboost_system.a pthread crypto cryptopp gtest
        gflags ssl mpirxx mpir sodium
        ${PROTOBUF_LIBRARY} ${SPDZ_HOME}/libSPDZ.a
        /usr/local/lib/libglog.so
        /usr/lib/x86_64-linux-gnu/libgmp.so
        /usr/lib/x86_64-linux-gnu/libgmpxx.so
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so)
