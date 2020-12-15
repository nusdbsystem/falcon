#!/bin/bash

OS=$1

. config_partyserver.properties
export Env=dev
export SERVICE_NAME=partyserver
export COORDINATOR_IP=$COORDINATOR_IP
export PARTY_SERVER_IP=$PARTY_SERVER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export PARTY_SERVER_NODE_PORT=$PARTY_SERVER_NODE_PORT

make $OS
./bin/falcon_platform