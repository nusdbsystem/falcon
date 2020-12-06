#!/bin/bash

. coordinator.properties

export SERVICE_NAME=$SERVICE_NAME
export COORDINATOR_IP=$COORDINATOR_IP
export LISTENER_IP=$LISTENER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export MS_ENGINE=$MS_ENGINE
export Env=$Env
./coordinator_server