#!/bin/bash

. config_coord.properties
export Env=dev
export SERVICE_NAME=coord
export COORDINATOR_IP=$COORDINATOR_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export MS_ENGINE=$MS_ENGINE
./coordinator_server