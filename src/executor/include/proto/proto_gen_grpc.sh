#!/bin/bash
SRC_DIR=v0/inference/
DST_DIR=../message/inference/
protoc -I=$SRC_DIR --cpp_out=$DST_DIR --grpc_out=$DST_DIR --plugin=protoc-gen-grpc=/home/wuyuncheng/.local/bin/grpc_cpp_plugin $SRC_DIR/lr_grpc.proto