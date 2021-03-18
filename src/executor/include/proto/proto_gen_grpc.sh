#!/bin/bash
SRC_DIR=v0/inference/
DST_DIR=../message/inference/
# gRPC binary path, default is to use the compiled gRPC at ~/.local
GRPC_BIN="/home/wuyuncheng/.local/bin/"
# alternatively use gRPC binary from vcpkg
# GRPC_BIN="/home/svd/Downloads/vcpkg/installed/x64-linux/tools/grpc/"
protoc -I=$SRC_DIR --cpp_out=$DST_DIR --grpc_out=$DST_DIR --plugin=protoc-gen-grpc=${GRPC_BIN}grpc_cpp_plugin $SRC_DIR/lr_grpc.proto