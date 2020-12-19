#!/bin/bash

OS=$1

. config_partyserver.properties
export Env=dev
export SERVICE_NAME=partyserver
export COORD_SERVER_IP=$COORD_SERVER_IP
export PARTY_SERVER_IP=$PARTY_SERVER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export PARTY_SERVER_NODE_PORT=$PARTY_SERVER_NODE_PORT
export PARTY_SERVER_ID=$PARTY_SERVER_ID

make $OS || exit 1
./bin/falcon_platform