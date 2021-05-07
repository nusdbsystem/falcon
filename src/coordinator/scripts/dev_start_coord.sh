#!/bin/bash

# exit on error
set -e

if [ "$#" -ne 0 ]; then
    echo "Illegal number of parameters"
    echo "Usage: bash dev_start_coord.sh"
    exit 1
fi

# from https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
# while [[ $# -gt 0 ]]
# do
# key="$1"

# case $key in
#     --partyNumber)
#     partyNumber="$2"
#     shift # past argument
#     shift # past value
#     ;;
#     *)    # unknown option
#     echo "No party number provided"
#     echo "Usage: bash dev_start_coord.sh --partyNumber <partyNumber>"
#     exit 1
#     shift # past argument
#     ;;
# esac
# done

# echo "partyNumber = ${partyNumber}"

# populate the environmental variables from
# the config_.properties files, such as paths IP and Ports
source config_coord.properties

# if Coordinator server base path is not supplied in the config.properties
# then use "./dev_test"
if [ $COORD_SERVER_BASEPATH ];then
	echo "COORD_SERVER_BASEPATH provided: $COORD_SERVER_BASEPATH"
else
   # create new group of sub-folders with each run
   TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss
   DEV_TEST_OUTDIR=./dev_test/Coord_${TIMESTAMP}

	export COORD_SERVER_BASEPATH=$DEV_TEST_OUTDIR
   echo "COORD_SERVER_BASEPATH NOT provided, will use ${COORD_SERVER_BASEPATH}"
fi

# set up the folder/sub-folders inside COORD_SERVER_BASEPATH
# first create COORD_SERVER_BASEPATH/ if not already exists
mkdir -p $COORD_SERVER_BASEPATH

export ENV="dev"
export SERVICE_NAME="coord"
export COORD_SERVER_IP=$COORD_SERVER_IP
export COORD_SERVER_PORT=$COORD_SERVER_PORT
export LOG_PATH=$COORD_SERVER_BASEPATH
export JOB_DATABASE=$JOB_DATABASE
export N_CONSUMER=$N_CONSUMER

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

# NOTE: need "2>&1" before "&"
# To redirect both stdout and stderr to the same file
./bin/falcon_platform > $COORD_SERVER_BASEPATH/Coord-console.log 2>&1 &

# store the process id in basepath
echo $! > ./dev_test/Coord.pid

echo "===== Done with Coordinator ====="
echo
