#!/bin/bash

clear
# exit on error
set -e

if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters"
    echo "Usage: bash dev_start_partyserver.sh --partyID <PARTY_ID>"
    exit 1
fi

# from https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --partyID)
    PARTY_ID="$2"
    shift # past argument
    shift # past value
    ;;
    *)    # unknown option
    echo "No party ID provided"
    echo "Usage: bash dev_start_partyserver.sh --partyID <PARTY_ID>"
    exit 1
    shift # past argument
    ;;
esac
done

echo "PARTY_ID = ${PARTY_ID}"

# populate the environmental variables from
# the config_.properties files, such as paths IP and Ports
source config_partyserver.properties

# if Party server base path is not supplied in the config.properties
# then use falcon_logs/
if [ $PARTY_SERVER_BASEPATH ];then
	echo "PARTY_SERVER_BASEPATH provided: $PARTY_SERVER_BASEPATH"
	DEV_LOG_PATH=$PARTY_SERVER_BASEPATH/falcon_logs/Party-${PARTY_ID}_${TIMESTAMP}
else
   # create new group of sub-folders with each run
   TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss
	 DEV_LOG_PATH=/opt/falcon/src/falcon_platform/falcon_logs/Party-${PARTY_ID}_${TIMESTAMP}
   echo "PARTY_SERVER_BASEPATH NOT provided, will use ${PARTY_SERVER_BASEPATH}"
fi

# store log to current falcon_logs
mkdir -p ./falcon_logs

TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss

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
export LOG_PATH=$DEV_LOG_PATH

# increment coordinator server port by partyserver ID
# party ID can be 0, so needs to add extra 1
let port=$PARTY_ID+$COORD_SERVER_PORT+1
echo using $port for PARTY_SERVER_NODE_PORT
export PARTY_SERVER_NODE_PORT=$port
export PARTY_ID=$PARTY_ID

export MPC_EXE_PATH=$MPC_EXE_PATH
export FL_ENGINE_PATH=$FL_ENGINE_PATH

# launch party server X
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
echo $! > ./falcon_logs/Party-${PARTY_ID}.pid

echo "===== Done with Party-${PARTY_ID} ====="
echo
