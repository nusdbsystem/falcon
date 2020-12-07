#!/bin/bash

. config_listener.properties
export Env=dev
export SERVICE_NAME=listener
export COORDINATOR_IP=$COORDINATOR_IP
export LISTENER_IP=$LISTENER_IP
export DATA_BASE_PATH=$LISTENER_BASE_PATH
./coordinator_server