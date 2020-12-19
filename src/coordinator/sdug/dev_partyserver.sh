#!/bin/bash

. config_partyserver.properties
export Env=dev
export SERVICE_NAME=partyserver
export COORD_SERVER_IP=$COORD_SERVER_IP
export PARTY_SERVER_IP=$PARTY_SERVER_IP

if [ ! -n "$1" ] ;then
      # start all services
     echo "No party number provided"
     exit 1
fi

export DATA_BASE_PATH="./.dev_test/party$1/logs/"
let "port=$1+30004"
export PARTY_SERVER_NODE_PORT=$port

OS=$2
if [ ! -n "$2" ] ;then
      # start all services
     echo "No OS provided, default to linux"
     OS=linux
fi

make build_$OS || exit 1
./bin/falcon_platform
