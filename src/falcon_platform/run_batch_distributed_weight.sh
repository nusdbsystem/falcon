#!/bin/bash

####### lime_weight nsample2000 #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w2.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-2
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w4.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-3
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w6.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-4
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w8.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-5
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w10.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-6
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w12.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-7
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w14.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-8
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample2000/three_parties_nsample2000_efficiency_w16.json
sleep 120
bash scripts/docker_service_rm_all_container.sh


####### lime_weight nsample4000 #######

# worker-9
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w2.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-10
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w4.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-11
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w6.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-12
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w8.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-13
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w10.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-14
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w12.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-15
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w14.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-16
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample4000/three_parties_nsample4000_efficiency_w16.json
sleep 120
bash scripts/docker_service_rm_all_container.sh


####### lime_weight nsample6000 #######

# worker-17
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w2.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-18
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w4.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-19
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w6.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-20
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w8.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-21
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w10.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-22
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w12.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-23
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w14.json
sleep 120
bash scripts/docker_service_rm_all_container.sh

# worker-24
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_weight/nsample6000/three_parties_nsample6000_efficiency_w16.json
sleep 120
bash scripts/docker_service_rm_all_container.sh