cmake_minimum_required(VERSION 3.10)
project(falcon_test)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")
# since the compiled executor at /opt/falcon is used for development debugging,
# defer the "prod" status and activate ASAN with helpful debug flags
# -g option gives more information
# -fno-omit-frame-pointer gets nicer stack traces in error messages
# -fno-common to make global variables protected by ASAN
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -fsanitize=address -g -fno-common -fno-omit-frame-pointer")

#link_directories(/usr/ssl/lib)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include/
        ${PROTOBUF_INCLUDE_DIR}
        ${SPDZ_HOME}
        ${SPDZ_HOME}/local/include
        ${PROJECT_SOURCE_DIR}/src)

set(TEST_SOURCE_FILES test_main.cc
        falcon/test_fixed_point.cc
        falcon/test_pb_converter.cc
        falcon/test_djcs_t_aux.cc
        falcon/test_io_util.cc
        falcon/test_model_io.cc)

add_executable(falcon_test ${TEST_SOURCE_FILES})

target_link_libraries(falcon_test
        /opt/falcon/third_party/libhcs/lib/libhcs.so
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
