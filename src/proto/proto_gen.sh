#!/bin/bash
SRC_DIR=v0/
DST_DIR=v0/message
protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto