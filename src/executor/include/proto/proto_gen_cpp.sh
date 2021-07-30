#!/bin/bash
SRC_DIR="/opt/falcon/src/executor/include/proto/v0"
DST_DIR="/opt/falcon/src/executor/include/message"
protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto