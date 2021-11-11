cmake_minimum_required(VERSION 3.10)
project(falcon)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -lboost_program_options -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -lboost_program_options")
# since the compiled executor at /opt/falcon is used for development debugging,
# defer the "prod" status and activate ASAN with helpful debug flags
# -g option gives more information
# -fno-omit-frame-pointer gets nicer stack traces in error messages
# -fno-common to make global variables protected by ASAN
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -lboost_program_options -fsanitize=address -g -fno-common -fno-omit-frame-pointer")
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
        ../../include/falcon/utils/pb_converter/alg_params_converter.h
        utils/pb_converter/alg_params_converter.cc
        ../../include/falcon/utils/metric/classification.h
        ../../include/falcon/utils/math/math_ops.h
        ../../include/falcon/utils/pb_converter/network_converter.h
        utils/pb_converter/network_converter.cc
        ../../include/falcon/utils/pb_converter/tree_converter.h
        utils/pb_converter/tree_converter.cc
        ../../include/falcon/utils/pb_converter/lr_converter.h
        utils/pb_converter/lr_converter.cc
        utils/metric/classification.cc
        utils/math/math_ops.cc
        ../../include/falcon/utils/base64.h
        utils/base64.cc
        include/message/common.pb.h
        include/message/common.pb.cc
        include/message/model.pb.h
        include/message/model.pb.cc
        include/message/phe_keys.pb.h
        include/message/phe_keys.pb.cc
        include/message/alg_params.pb.h
        include/message/alg_params.pb.cc
        include/message/network.pb.h
        include/message/network.pb.cc
        include/message/ps_network.pb.h
        include/message/ps_network.pb.cc
        include/message/tree.pb.h
        include/message/tree.pb.cc
        include/message/lr.pb.h
        include/message/lr.pb.cc
        ../../include/falcon/operator/phe/fixed_point_encoder.h
        operator/phe/fixed_point_encoder.cc
        ../../include/falcon/operator/phe/djcs_t_aux.h
        operator/phe/djcs_t_aux.cc
        ../../include/falcon/operator/mpc/spdz_connector.h
        operator/mpc/spdz_connector.cc
        ../../include/falcon/utils/io_util.h
        utils/io_util.cc
        ../../include/falcon/algorithm/vertical/linear_model/logistic_regression_builder.h
        algorithm/vertical/linear_model/logistic_regression_builder.cc
        ../../include/falcon/algorithm/vertical/linear_model/logistic_regression_model.h
        algorithm/vertical/linear_model/logistic_regression_model.cc
        ../../include/falcon/algorithm/model_builder.h
        algorithm/model_builder.cc
        ../../include/falcon/model/model_io.h
        model/model_io.cc
        ../../include/falcon/inference/server/inference_server.h
        inference/server/inference_server.cc
        ../../include/falcon/inference/server/lr_inference_service.h
        inference/server/lr_inference_service.cc
        ../../include/falcon/inference/server/dt_inference_service.h
        inference/server/dt_inference_service.cc
        ../../include/falcon/inference/server/rf_inference_service.h
        inference/server/rf_inference_service.cc
        ../../include/falcon/algorithm/vertical/tree/node.h
        algorithm/vertical/tree/node.cc
        ../../include/falcon/algorithm/vertical/tree/feature.h
        algorithm/vertical/tree/feature.cc
        ../../include/falcon/algorithm/vertical/tree/tree_model.h
        algorithm/vertical/tree/tree_model.cc
        ../../include/falcon/algorithm/vertical/tree/tree_builder.h
        algorithm/vertical/tree/tree_builder.cc
        ../../include/falcon/algorithm/vertical/tree/forest_builder.h
        algorithm/vertical/tree/forest_builder.cc
        ../../include/falcon/algorithm/vertical/tree/forest_model.h
        algorithm/vertical/tree/forest_model.cc
        ../../include/falcon/algorithm/vertical/tree/gbdt_builder.h
        algorithm/vertical/tree/gbdt_builder.cc
        ../../include/falcon/algorithm/vertical/tree/gbdt_model.h
        algorithm/vertical/tree/gbdt_model.cc
        ../../include/falcon/algorithm/vertical/tree/gbdt_loss.h
        algorithm/vertical/tree/gbdt_loss.cc
        ../../include/falcon/inference/server/gbdt_inference_service.h
        inference/server/gbdt_inference_service.cc
        ../../include/falcon/distributed/parameter_server_base.h
        ../../include/falcon/distributed/worker.h
        distributed/worker.cc
        distributed/parameter_server_base.cc
        ../../include/falcon/algorithm/vertical/linear_model/logistic_regression_ps.h
        algorithm/vertical/linear_model/logistic_regression_ps.cc
        ../../include/falcon/utils/logger/logger.h
        utils/logger/logger.cc
        ../../include/falcon/algorithm/vertical/linear_model/linear_regression_builder.h
        ../../include/falcon/algorithm/vertical/linear_model/linear_regression_model.h
        ../../include/falcon/inference/interpretability/lime/lime.h
        ../../include/falcon/utils/pb_converter/interpretability_converter.h
        algorithm/vertical/linear_model/linear_regression_builder.cc
        algorithm/vertical/linear_model/linear_regression_model.cc
        inference/interpretability/lime/lime.cc
        utils/pb_converter/interpretability_converter.cc
        include/message/interpretability.pb.h
        include/message/interpretability.pb.cc)

set(SPDZ_HOME "/opt/falcon/third_party/MP-SPDZ/")
set(FALCON_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/")

# Indicate which source files should be included
add_library(executor ${SOURCE_FILES})

# Need to Revise protobuf version to 3.14 that is compatible with grpc;
include(FindProtobuf)
find_package(Protobuf REQUIRED)

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
#set(protobuf_MODULE_COMPATIBLE TRUE)
#find_package(Protobuf CONFIG REQUIRED)
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
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

find_package(Boost COMPONENTS program_options REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SERVED_PKG REQUIRED IMPORTED_TARGET served)

# Add include directories to the target executor.
target_include_directories(executor PUBLIC
        ${Boost_INCLUDE_DIRS}
        ${FALCON_INCLUDE_DIR}
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        /opt/falcon/include)

target_link_libraries(executor
        /opt/falcon/third_party/libhcs/lib/libhcs.so
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
        /usr/lib/x86_64-linux-gnu/libglog.so
        /usr/lib/x86_64-linux-gnu/libgmp.so
        /usr/lib/x86_64-linux-gnu/libgmpxx.so
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

#find_package (glog 0.4.0 REQUIRED)

# Add directories in which the linker will look for libraries.
link_directories(/usr/local/lib /usr/ssl/lib ${SPDZ_HOME}/local/lib)

# Add include directories to the build.
include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include/
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        ${PROJECT_SOURCE_DIR}/src)

add_executable(falcon main.cc)

target_link_libraries(falcon
        /opt/falcon/third_party/libhcs/lib/libhcs.so
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
        #mpirxx
        #mpir
        sodium
        /usr/lib/x86_64-linux-gnu/libglog.so
        /usr/lib/x86_64-linux-gnu/libgmp.so
        /usr/lib/x86_64-linux-gnu/libgmpxx.so
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        PkgConfig::SERVED_PKG)

# add_subdirectory(inference/client)