#!/bin/bash

####### vary worker num #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w20.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w25.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w30.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w35.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w40.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w45.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w50.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w55.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w60.json
sleep 2000
bash scripts/docker_service_rm_all_container.sh
