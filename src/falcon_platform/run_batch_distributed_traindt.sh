#!/bin/bash

####### lime train dt feature 20 #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w1.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w2.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w3.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w4.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w5.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w6.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w7.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w8.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w9.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature20/three_parties_partyfeature20_efficiency_dt_w10.json
sleep 480
bash scripts/docker_service_rm_all_container.sh


####### lime train dt feature 30 #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w1.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w2.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w3.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w4.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w5.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w6.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w7.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w8.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w9.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature30/three_parties_partyfeature30_efficiency_dt_w10.json
sleep 480
bash scripts/docker_service_rm_all_container.sh



####### lime train dt feature 40 #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w1.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w2.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w3.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w4.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w5.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w6.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w7.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w8.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w9.json
sleep 480
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_train_dt/partyfeature40/three_parties_partyfeature40_efficiency_dt_w10.json
sleep 480
bash scripts/docker_service_rm_all_container.sh
