#!/bin/bash

# exit on error
set -e

# from https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --partyNumber)
    partyNumber="$2"
    shift # past argument
    shift # past value
    ;;
    *)    # unknown option
    echo "No party number provided"
    echo "Usage: bash dev_start_coord.sh --partyNumber <partyNumber>"
    exit 1
    shift # past argument
    ;;
esac
done

echo "partyNumber = ${partyNumber}"

# mkdir for relevant resources
bash scripts/create_folder.sh $partyNumber

# launch coordinator
source config_coord.properties

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

export Env=dev
export SERVICE_NAME=coord
export COORD_SERVER_IP=$COORD_SERVER_IP
export COORD_SERVER_PORT=$COORD_SERVER_PORT
export BASE_PATH=$BASE_PATH
export JOB_DATABASE=$JOB_DATABASE

make $makeOS
./bin/falcon_platform
