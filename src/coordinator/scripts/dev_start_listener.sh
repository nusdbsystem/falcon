#!/bin/bash

OS=$1

. config_listener.properties
export Env=dev
export SERVICE_NAME=listener
export COORDINATOR_IP=$COORDINATOR_IP
export LISTENER_IP=$LISTENER_IP
export DATA_BASE_PATH=$LISTENER_BASE_PATH

make $OS
./bn/coordinator_server