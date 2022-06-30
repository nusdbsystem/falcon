#!/bin/bash

clear

source config_coord.properties

# if Coordinator server rpcbase path is not supplied in the config.properties
# then use "./falcon_logs"
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

# launch coordinator
# detect the OS type with uname
makeOS=''
unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   makeOS='build_linux'
elif [[ "$unamestr" == 'Darwin' ]]; then
   makeOS='build_mac'
elif [[ "$unamestr" == 'WindowsNT' ]]; then
   makeOS='build_windows'
fi

make $makeOS
./bin/falcon_platform
