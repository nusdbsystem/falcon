#!/bin/bash

####### lime train dt feature 20 #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w1.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-2
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w2.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-3
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w3.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-4
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w4.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-5
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w5.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-6
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w6.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-7
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w7.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-8
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w8.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-9
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w9.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh

# worker-10
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w10.json
sleep 1500
bash scripts/docker_service_rm_all_container.sh


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


####### lime train dt feature 40 #######

# worker-21
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w1.json
sleep 2400
bash scripts/docker_service_rm_all_container.sh

# worker-22
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w2.json
sleep 2400
bash scripts/docker_service_rm_all_container.sh

# worker-23
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w3.json
sleep 2100
bash scripts/docker_service_rm_all_container.sh

# worker-24
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w4.json
sleep 2100
bash scripts/docker_service_rm_all_container.sh

# worker-25
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w5.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-26
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w6.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-27
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w7.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-28
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w8.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-29
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w9.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh

# worker-30
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w10.json
sleep 1800
bash scripts/docker_service_rm_all_container.sh