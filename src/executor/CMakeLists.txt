cmake_minimum_required(VERSION 3.10)
project(falcon)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

set(SOURCE_FILES
        network/Comm.hpp
        network/Comm.cpp
        network/ConfigFile.hpp
        network/ConfigFile.cpp
        party/party.h
        party/party.cc
        utils/pb_converter/model_pb_converter.h
        utils/pb_converter/model_pb_converter.cc
        include/message/model.pb.h
        include/message/model.pb.cc
        ../../include/falcon/operator/phe/fixed_point_encoder.h
        operator/phe/fixed_point_encoder.cc
        ../../include/falcon/operator/phe/djcs_t_aux.h
        operator/phe/djcs_t_aux.cc
        ../../include/falcon/operator/mpc/spdz_connector.h
        operator/mpc/spdz_connector.cc)

#set(SPDZ_HOME "$ENV{HOME}/Documents/MP-SPDZ/")
set(SPDZ_HOME "$ENV{HOME}/Documents/falcon/third_party/MP-SPDZ/")
set(FALCON_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/")

add_library(executor ${SOURCE_FILES})

include(FindProtobuf)
find_package(Protobuf REQUIRED)

target_include_directories(executor PUBLIC
        $ENV{HOME}/Documents/projects/boost_1_64_0/stage/lib
        ${FALCON_INCLUDE_DIR}
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        $ENV{HOME}/Documents/falcon/include)

target_link_libraries(executor
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
        ${PROTOBUF_LIBRARY}
        ${SPDZ_HOME}/libSPDZ.a
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

#find_package (glog 0.4.0 REQUIRED)

link_directories(/usr/local/lib /usr/ssl/lib ${SPDZ_HOME}/local/lib)

include_directories(
        $ENV{HOME}/Documents/projects/boost_1_64_0/stage/lib
        ${PROJECT_SOURCE_DIR}/include/
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        ${PROJECT_SOURCE_DIR}/src)

add_executable(falcon main.cc)

target_link_libraries(falcon
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