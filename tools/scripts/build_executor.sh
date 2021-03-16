#!/bin/bash
set -e
set -x

# for dev environment, install the executor at /opt/falcon
cd /opt/falcon

rm -rf build/

mkdir build
cmake -B build -H.

# try with vcpkg for gRPC
# cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/svd/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake --build build

cd build/

make

# for rebuilding of executor
# if updates only involve executor c++ code
# just cd build/ && make
