#!/bin/bash

OS=$1

. config_partyserver.properties
export Env=dev
export SERVICE_NAME=partyserver
export COORDINATOR_IP=$COORDINATOR_IP
export PARTYSERVER_IP=$PARTYSERVER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export PARTYSERVER_NODE_PORT=$PARTYSERVER_NODE_PORT

make $OS
./bin/coordinator_server