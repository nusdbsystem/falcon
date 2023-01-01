#!/bin/bash

clear
# exit on error
set -e

PARTY_ID="$2"
echo "PARTY_ID = ${PARTY_ID}"

MY_PATH=$(dirname "$0")
echo "$MY_PATH"

# populate the environmental variables from
# the config_.properties files, such as paths IP and Ports
source $MY_PATH/config_partyserver.properties

TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss
echo "PARTY_SERVER_BASEPATH provided: $PARTY_SERVER_BASEPATH"
USED_LOG_PATH=$PARTY_SERVER_BASEPATH/falcon_logs/Party-${PARTY_ID}_${TIMESTAMP}
mkdir -p "$USED_LOG_PATH"

# decide which deployment the partyServer will use to spawn worker
#export ENV="subprocess"
export ENV="docker"
#export IS_DEBUG="debug-on"
export IS_DEBUG="debug-off"
export SERVICE_NAME="partyserver"
export COORD_SERVER_IP=$COORD_SERVER_IP
export COORD_SERVER_PORT=$COORD_SERVER_PORT
export PARTY_SERVER_IP=$PARTY_SERVER_IP
export PARTY_SERVER_CLUSTER_IPS=$PARTY_SERVER_CLUSTER_IPS
export PARTY_SERVER_CLUSTER_LABEL=$PARTY_SERVER_CLUSTER_LABEL
export LOG_PATH=$USED_LOG_PATH

export PARTY_ID=$PARTY_ID
export PARTY_SERVER_NODE_PORT=$PARTY_SERVER_NODE_PORT

export MPC_EXE_PATH=$MPC_EXE_PATH
export FL_ENGINE_PATH=$FL_ENGINE_PATH
export FALCON_WORKER_IMAGE=$FALCON_WORKER_IMAGE

# launch party server X # detect the OS type with uname
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
