#!/bin/bash

####### lime train dt feature 30 #######

# worker-11
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w1.json
sleep 2100
bash scripts/docker_service_rm_all_container.sh

# worker-12
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w2.json
sleep 2100
bash scripts/docker_service_rm_all_container.sh

# worker-13
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w3.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-14
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w4.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-15
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w5.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-16
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w6.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-17
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w7.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-18
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w8.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-19
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w9.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-20
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w10.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

