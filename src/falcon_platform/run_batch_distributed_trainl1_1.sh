#!/bin/bash

####### lime train l1 batch128 #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w1.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w2.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-2
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w3.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-3
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w4.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-4
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w5.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-5
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w6.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-6
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w7.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-7
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w8.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-8
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w9.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-9
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l1/batch128/three_parties_batch128_efficiency_w10.json
sleep 360
bash scripts/docker_service_rm_all_container.sh
