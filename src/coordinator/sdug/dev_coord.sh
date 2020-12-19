#!/bin/bash

partyNumber=$1

bash sdug/create_folder.sh $partyNumber
bash scripts/dev_start_coord.sh $2