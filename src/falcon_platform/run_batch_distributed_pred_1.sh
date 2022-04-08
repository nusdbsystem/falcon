#!/bin/bash

####### lime pred lr #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w1.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w2.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w3.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w4.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w5.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w6.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w7.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w8.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w9.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/logistic/three_parties_lime_partyfeature20_lr_pred_w10.json
sleep 240
bash scripts/docker_service_rm_all_container.sh



####### lime pred rf #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w1.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w2.json
sleep 360
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w3.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w4.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w5.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w6.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w7.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w8.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w9.json
sleep 240
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/distributed/lime_pred/rf/three_parties_lime_partyfeature20_rf_pred_w10.json
sleep 240
bash scripts/docker_service_rm_all_container.sh