#!/bin/bash

# exit on error
set -e

# from https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --partyID)
    PARTY_SERVER_ID="$2"
    shift # past argument
    shift # past value
    ;;
    *)    # unknown option
    echo "No party ID provided"
    echo "Usage: bash dev_start_partyserver.sh --partyID <PARTY_SERVER_ID>"
    exit 1
    shift # past argument
    ;;
esac
done

echo "PARTY_SERVER_ID = ${PARTY_SERVER_ID}"

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

export Env=dev
export SERVICE_NAME=partyserver
export COORD_SERVER_IP=$COORD_SERVER_IP
export COORD_SERVER_PORT=$COORD_SERVER_PORT
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
