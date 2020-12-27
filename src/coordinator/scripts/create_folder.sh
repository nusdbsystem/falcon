#!/bin/bash

partyNumber=$1
rm -r dev_test

mkdir dev_test/

# setup coord folder
mkdir dev_test/coord

# setup party 1-N folders
for k in $(seq 1 $partyNumber)
do
    echo "making dir, $k"
    mkdir dev_test/party$k
    mkdir dev_test/party${k}/logs
    mkdir dev_test/party${k}/data_input
    mkdir dev_test/party${k}/data_output
    mkdir dev_test/party${k}/trained_models
    tree dev_test/
done

# sudo chmod -R 777 dev_test