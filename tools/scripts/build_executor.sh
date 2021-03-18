#!/bin/bash
set -e
set -x

# for dev environment, install the executor at /opt/falcon
cd /opt/falcon

rm -rf build/

mkdir build

# two ways to get gRPC for CMake
# 1. Default is to use the gRPC compiled from source (1.35.0)
# 2. Or use the gRPC from vcpkg (1.33.1)
# If using the gRPC compiled from source, no need to supply cmd flag
# If using vcpkg, append "--vcpkg" flag
if [[ $* == *--vcpkg* ]]
then
    # try with vcpkg for gRPC
    VCPKG_DIR="/home/svd/Downloads/"
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build
else
    # Default no flag in cmd, use compiled gRPC
    cmake -B build -H.
fi

cd build/

make

# for rebuilding of executor
# if updates only involve executor c++ code
# just cd build/ && make
