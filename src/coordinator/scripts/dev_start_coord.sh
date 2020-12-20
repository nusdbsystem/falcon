#!/bin/bash

. config_coord.properties

OS=$1
if [ ! -n "$1" ] ;then
      # start all services
     echo "No OS provided, default to linux"
     OS=linux
fi

export Env=dev
export SERVICE_NAME=coord
export COORD_SERVER_IP=$COORD_SERVER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export JOB_DATABASE=$JOB_DATABASE

make build_$OS || exit 1
./bin/falcon_platform