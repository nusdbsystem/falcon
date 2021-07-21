# Falcon
Falcon is a federated learning system with privacy protection. It allows
multiple parties to collaboratively train a machine learning model and 
serve for new requests.

## Prerequisites
* Ubuntu 18.04

* install partially homomorphic encryption library **libhcs**.

* install secure multiparty computation library **MP-SPDZ**.

* install google remote procedure call library **GRPC**. (`<= 1.33.1` with protoc `3.14.0`)

## Quick Setup

* build the falcon_platform

```shell script
# launch the coordinator and partyserver 0~2
bash src/falcon_platform/scripts/dev_start_all.sh --partyCount 3
```

* build the executor

```shell script
# start the 3 semi-party.x simulating 3 parties
bash src/falcon_platform/scripts/start_semi-party012.sh

# build falcon executor
bash tools/scripts/build_executor.sh
```

* prepare data and json file for training or inference (default dataset
is under data/dataset/bank_marketing_data/ and default job is under
src/falcon_platform/train_jobs/three_parties_train_job.json)

* submit train job to coordinator
```shell script
cd src/falcon_platform
# UCI tele-marketing bank dataset
python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path ./train_jobs/three_parties_train_job_banktele.json
# UCI breast cancer dataset
python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path ./train_jobs/three_parties_train_job_breastcancer.json
```

* after training, find the saved model and report under 
data/dataset/bank_marketing_data/client0/

* terminate coordinator and partyserver
```shell script
bash src/falcon_platform/scripts/dev_terminate_all.sh --partyCount 3
```