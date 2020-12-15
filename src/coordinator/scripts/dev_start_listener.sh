#!/bin/bash

OS=$1

. config_listener.properties
export Env=dev
export SERVICE_NAME=listener
export COORDINATOR_IP=$COORDINATOR_IP
export LISTENER_IP=$LISTENER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export LISTENER_NODE_PORT=$LISTENER_NODE_PORT

make $OS
./bin/coordinator_server