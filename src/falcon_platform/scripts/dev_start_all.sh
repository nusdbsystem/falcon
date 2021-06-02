#!/bin/bash

# exit on error
set -e

if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters"
    echo "Usage: bash dev_start_all.sh --partyCount <PARTY_COUNT>"
    exit 1
fi

# from https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --partyCount)
    PARTY_COUNT="$2"
    shift # past argument
    shift # past value
    ;;
    *)    # unknown option
    echo "No party count provided"
    echo "Usage: bash dev_start_all.sh --partyCount <PARTY_COUNT>"
    exit 1
    shift # past argument
    ;;
esac
done

echo "PARTY_COUNT = ${PARTY_COUNT}"

# Setup falcon_platform

# Launch coordinator first:
bash scripts/dev_start_coord.sh

# Setup partyserver 0~(PARTY_COUNT-1)
# Party ID must start with 0
for (( c=0; c<$PARTY_COUNT; c++ ))
do
  echo "Launch Party $c..."
  bash scripts/dev_start_partyserver.sh --partyID $c
done

echo DONE
