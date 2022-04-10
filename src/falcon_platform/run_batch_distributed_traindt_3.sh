#!/bin/bash

####### lime train dt feature 40 #######

# worker-23
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w3.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-24
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w4.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-25
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w5.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-26
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w6.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-27
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w7.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-28
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w8.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-29
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w9.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-30
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w10.json
sleep 600
bash scripts/docker_service_rm_all_container.sh
