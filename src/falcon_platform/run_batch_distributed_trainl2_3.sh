#!/bin/bash

####### lime train l2 batch1024 #######

# worker-21
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w1.json
sleep 1200
bash scripts/docker_service_rm_all_container.sh

# worker-22
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w2.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-23
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w3.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-24
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w4.json
sleep 300
bash scripts/docker_service_rm_all_container.sh

# worker-25
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w5.json
sleep 300
bash scripts/docker_service_rm_all_container.sh

# worker-26
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w6.json
sleep 300
bash scripts/docker_service_rm_all_container.sh

# worker-27
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w7.json
sleep 300
bash scripts/docker_service_rm_all_container.sh

# worker-28
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w8.json
sleep 300
bash scripts/docker_service_rm_all_container.sh

# worker-29
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w9.json
sleep 300
bash scripts/docker_service_rm_all_container.sh

# worker-30
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_l2/batch1024/three_parties_batch1024_efficiency_w10.json
sleep 300
bash scripts/docker_service_rm_all_container.sh
