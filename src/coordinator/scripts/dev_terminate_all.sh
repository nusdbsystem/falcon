#!/bin/bash

# set -x

# exit on error
# set -e

if [ "$#" -ne 2 ]; then
    echo "Illegal number of parameters"
    echo "Usage: bash dev_terminate_all.sh --partyCount <PARTY_COUNT>"
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
    echo "Usage: bash dev_terminate_all.sh --partyCount <PARTY_COUNT>"
    exit 1
    shift # past argument
    ;;
esac
done

echo "PARTY_COUNT = ${PARTY_COUNT}"

# terminate the coordinator
echo "Terminate Coordinator..."
kill -9 $(cat dev_test/Coord.pid)

for (( c=0; c<$PARTY_COUNT; c++ ))
do
  echo "Terminate Party $c..."
  kill -9 $(cat dev_test/Party-$c.pid)
done

# just in case, kill after grep for keyword
# The grep filters that based on your search string,
# [x] is a trick to stop you picking up the actual grep process itself.
# ref: https://stackoverflow.com/a/3510850
kill $(ps aux | grep '[f]alcon_platform' | awk '{print $2}')

echo DONE
