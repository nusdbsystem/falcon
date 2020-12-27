#!/bin/bash

partyNumber=$1
rm -r dev_test

mkdir dev_test/

# setup coord folder
echo "creating folders for coordinator"
mkdir dev_test/coord

# setup party 1-N folders
for k in $(seq 1 $partyNumber)
do
    echo "creating folders for party-$k"
    mkdir dev_test/party$k
    mkdir dev_test/party${k}/logs
    mkdir dev_test/party${k}/data_input
    mkdir dev_test/party${k}/data_output
    mkdir dev_test/party${k}/trained_models
done

tree dev_test/

# sudo chmod -R 777 dev_test