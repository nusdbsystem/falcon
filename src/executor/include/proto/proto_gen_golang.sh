#!/bin/bash
SRC_DIR="/opt/falcon/src/executor/include/proto/v0"
DST_DIR="/opt/falcon/src/falcon_platform"

# for alg_params.pb.go
protoc -I=$SRC_DIR --go_out=$DST_DIR $SRC_DIR/alg_params.proto
# for network.pb.go
protoc -I=$SRC_DIR --go_out=$DST_DIR $SRC_DIR/network.proto
