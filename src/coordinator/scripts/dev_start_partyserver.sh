#!/bin/bash

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

. config_partyserver.properties

export Env=dev
export SERVICE_NAME=partyserver
export COORD_SERVER_IP=$COORD_SERVER_IP
export PARTY_SERVER_IP=$PARTY_SERVER_IP
export WORK_BASE_PATH=$WORK_BASE_PATH
export PARTY_SERVER_NODE_PORT=$PARTY_SERVER_NODE_PORT
export PARTY_SERVER_ID=$PARTY_SERVER_ID

make $makeOS
./bin/falcon_platform
