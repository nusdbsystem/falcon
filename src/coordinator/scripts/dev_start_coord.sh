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

# set up the folder/sub-folders inside dev_test
# first create dev_test/ if not already exists
mkdir -p dev_test/

# create new group of sub-folders with each run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)  # for hh:mm:ss
DEV_TEST_OUTDIR=dev_test/Coord_${TIMESTAMP}
# setup coord folder
mkdir -p $DEV_TEST_OUTDIR

# populate the environmental variables from
# the config_.properties files, such as paths IP and Ports
source config_coord.properties

export Env=dev
export SERVICE_NAME=coord
export COORD_SERVER_IP=$COORD_SERVER_IP
export COORD_SERVER_PORT=$COORD_SERVER_PORT
export BASE_PATH=$DEV_TEST_OUTDIR
export JOB_DATABASE=$JOB_DATABASE

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
./bin/falcon_platform > $DEV_TEST_OUTDIR/Coord-console.log &
echo $! > dev_test/Coord.pid
