#!/bin/bash

if [ ! -n "$1" ] ;then
     echo "No party number provided"
     echo "Usage: bash dev_start_coord.sh <partyNumber>"
     exit 1
fi

partyNumber=$1

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
export BASE_PATH=$BASE_PATH
export JOB_DATABASE=$JOB_DATABASE

make $makeOS
./bin/falcon_platform
