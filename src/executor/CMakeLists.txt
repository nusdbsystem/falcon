cmake_minimum_required(VERSION 3.10)
project(falcon)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -lboost_program_options -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -lboost_program_options")

set(SOURCE_FILES
        ../../include/falcon/network/Comm.hpp
        network/Comm.cpp
        ../../include/falcon/network/ConfigFile.hpp
        network/ConfigFile.cpp
        ../../include/falcon/party/party.h
        party/party.cc
        ../../include/falcon/utils/pb_converter/model_converter.h
        utils/pb_converter/model_converter.cc
        ../../include/falcon/utils/pb_converter/phe_keys_converter.h
        utils/pb_converter/phe_keys_converter.cc
        ../../include/falcon/utils/pb_converter/common_converter.h
        utils/pb_converter/common_converter.cc
        ../../include/falcon/utils/pb_converter/lr_params_converter.h
        utils/pb_converter/lr_params_converter.cc
        ../../include/falcon/utils/metric/accuracy.h
        ../../include/falcon/utils/pb_converter/network_converter.h
        utils/pb_converter/network_converter.cc
        utils/metric/accuracy.cc
        ../../include/falcon/utils/base64.h
        utils/base64.cc
        include/message/common.pb.h
        include/message/common.pb.cc
        include/message/model.pb.h
        include/message/model.pb.cc
        include/message/phe_keys.pb.h
        include/message/phe_keys.pb.cc
        include/message/lr_params.pb.h
        include/message/lr_params.pb.cc
        include/message/network.pb.h
        include/message/network.pb.cc
        include/message/inference/lr_grpc.pb.h
        include/message/inference/lr_grpc.pb.cc
        include/message/inference/lr_grpc.grpc.pb.h
        include/message/inference/lr_grpc.grpc.pb.cc
        ../../include/falcon/operator/phe/fixed_point_encoder.h
        operator/phe/fixed_point_encoder.cc
        ../../include/falcon/operator/phe/djcs_t_aux.h
        operator/phe/djcs_t_aux.cc
        ../../include/falcon/operator/mpc/spdz_connector.h
        operator/mpc/spdz_connector.cc
        ../../include/falcon/utils/io_util.h
        utils/io_util.cc
        ../../include/falcon/algorithm/vertical/linear_model/logistic_regression.h
        algorithm/vertical/linear_model/logistic_regression.cc
        ../../include/falcon/algorithm/model.h
        algorithm/model.cc
        ../../include/falcon/model/model_io.h
        model/model_io.cc
        ../../include/falcon/inference/server/inference_server.h
        inference/server/inference_server.cc)

#set(SPDZ_HOME "$ENV{HOME}/Documents/MP-SPDZ/")
set(SPDZ_HOME "$ENV{HOME}/Documents/falcon/third_party/MP-SPDZ/")
set(FALCON_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/")

add_library(executor ${SOURCE_FILES})

#include(FindProtobuf)
#find_package(Protobuf REQUIRED)
# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
message(STATUS "Using protobuf ${Protobuf_VERSION}")

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    add_definitions(-D_WIN32_WINNT=0x600)
endif()

find_package(Threads REQUIRED)

# This branch assumes that gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

find_package(Boost COMPONENTS program_options REQUIRED)

target_include_directories(executor PUBLIC
        ${Boost_INCLUDE_DIRS}
        ${FALCON_INCLUDE_DIR}
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        $ENV{HOME}/Documents/falcon/include)

target_link_libraries(executor
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
        #${PROTOBUF_LIBRARY}
        ${Boost_LIBRARIES}
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
        /usr/lib/x86_64-linux-gnu/libgmpxx.so
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

#find_package (glog 0.4.0 REQUIRED)

link_directories(/usr/local/lib /usr/ssl/lib ${SPDZ_HOME}/local/lib)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include/
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        ${PROJECT_SOURCE_DIR}/src)

add_executable(falcon main.cc)

target_link_libraries(falcon
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
        #${PROTOBUF_LIBRARY}
        ${Boost_LIBRARIES}
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
        /usr/lib/x86_64-linux-gnu/libgmpxx.so
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

add_subdirectory(inference/client)