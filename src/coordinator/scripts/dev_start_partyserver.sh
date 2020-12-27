#!/bin/bash

if [ ! -n "$1" ] ;then
     echo "No partyserver ID provided"
     echo "Usage: bash dev_start_partyserver.sh <PARTY_SERVER_ID>"
     exit 1
fi

export PARTY_SERVER_ID=$1
# echo $PARTY_SERVER_ID

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

source config_partyserver.properties

COORD_SERVER_PORT=30004

export Env=dev
export SERVICE_NAME=partyserver
export COORD_SERVER_IP=$COORD_SERVER_IP
export PARTY_SERVER_IP=$PARTY_SERVER_IP
# export BASE_PATH=$BASE_PATH
# export PARTY_SERVER_NODE_PORT=$PARTY_SERVER_NODE_PORT
export BASE_PATH="./dev_test/party$PARTY_SERVER_ID/logs/"
# increment coordinator server port by partyserver ID
let port=$PARTY_SERVER_ID+$COORD_SERVER_PORT
echo using $port for PARTY_SERVER_NODE_PORT
export PARTY_SERVER_NODE_PORT=$port
export PARTY_SERVER_ID=$PARTY_SERVER_ID

make $makeOS
./bin/falcon_platform
