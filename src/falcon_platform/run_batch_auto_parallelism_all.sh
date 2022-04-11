#!/bin/bash

####### vary worker num #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w20.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w22.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w24.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w26.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w28.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w30.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w32.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w34.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w36.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w38.json
sleep 600
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000/three_parties_partyfeature20_efficiency_lr_l1_w40.json
sleep 600
bash scripts/docker_service_rm_all_container.sh



####### vary worker num #######

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w20.json
sleep 1200
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w22.json
sleep 1200
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w24.json
sleep 1200
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w26.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w28.json
sleep 1000
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w30.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w32.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w34.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w36.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w38.json
sleep 800
bash scripts/docker_service_rm_all_container.sh

# worker-1
python3 coordinator_client.py --url 10.0.0.80:30004 -method submit -path ./examples/train_job_dsls/sigmod_experiments/efficiency/auto_parallelism/nSample4000Baseline/three_parties_partyfeature20_efficiency_lr_l1_w40.json
sleep 800
bash scripts/docker_service_rm_all_container.sh