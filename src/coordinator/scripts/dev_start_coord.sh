#!/bin/bash

. config_coord.properties

OS=$1

export Env=dev
export SERVICE_NAME=coord
export COORD_SERVER_IP=$COORD_SERVER_IP
export DATA_BASE_PATH=$DATA_BASE_PATH
export JOB_DB_ENGINE=$JOB_DB_ENGINE

make $OS || exit 1
./bin/falcon_platform