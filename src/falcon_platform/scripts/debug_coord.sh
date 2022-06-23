#!/bin/bash

clear
# exit on error
set -e

if [ "$#" -ne 0 ]; then
    echo "Illegal number of parameters"
    echo "Usage: bash dev_start_coord.sh"
    exit 1
fi

# populate the environmental variables from
# the config_.properties files, such as paths IP and Ports
source config_coord.properties

# if Coordinator server rpcbase path is not supplied in the config.properties
# then use "./falcon_logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss
if [ $COORD_SERVER_BASEPATH ];then
	echo "COORD_SERVER_BASEPATH provided: $COORD_SERVER_BASEPATH"
	USED_LOG_PATH=$COORD_SERVER_BASEPATH/falcon_logs/Coord_${TIMESTAMP}
else
   # create new group of sub-folders with each run
   USED_LOG_PATH=/opt/falcon/src/falcon_platform/falcon_logs/Coord_${TIMESTAMP}
   echo "COORD_SERVER_BASEPATH NOT provided, will log to this path ${USED_LOG_PATH}"
fi

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

# not run in background, used when debug
./bin/falcon_platform

# store the process id in basepath
echo $! > ./falcon_logs/Coord.pid

echo "===== Done with Coordinator ====="
echo