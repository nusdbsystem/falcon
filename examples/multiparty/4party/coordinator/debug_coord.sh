#!/bin/bash

clear
# exit on error
set -e

MY_PATH=$(dirname "$0")
echo "$MY_PATH"

source $MY_PATH/config_coord.properties

TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss
USED_LOG_PATH=$COORD_SERVER_BASEPATH/falcon_logs/Coord_${TIMESTAMP}
mkdir -p "$USED_LOG_PATH"

export IS_DEBUG="debug-off"
export SERVICE_NAME="coord"
export COORD_SERVER_IP=$COORD_SERVER_IP
export COORD_SERVER_PORT=$COORD_SERVER_PORT
export LOG_PATH=$USED_LOG_PATH
export JOB_DATABASE=$JOB_DATABASE
export N_CONSUMER=$N_CONSUMER
export FALCON_WORKER_IMAGE=$FALCON_WORKER_IMAGE
export COORD_SERVER_BASEPATH=$USED_LOG_PATH

# launch coordinator detect the OS type with uname
source examples/source_cluster_go_env.sh
makeOS=''
unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   makeOS='build_linux'
elif [[ "$unamestr" == 'Darwin' ]]; then
   makeOS='build_mac'
elif [[ "$unamestr" == 'WindowsNT' ]]; then
   makeOS='build_windows'
fi

cd src/falcon_platform/
make $makeOS
./bin/falcon_platform

