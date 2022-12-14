# Falcon

Falcon is a federated learning system with privacy protection. It allows
multiple parties to collaboratively train a machine learning model and
serve for new requests.

## Prerequisites

* Ubuntu 18.04

* install partially homomorphic encryption library **libhcs**.

* install secure multiparty computation library **MP-SPDZ**.

* install web server request handling library **[served](https://github.com/meltwater/served)**.

# Develop guide

### Build the platform

Current development follows the current patterns:

1. Edit falcon or `MPC`  locally and `git commit` to corresponding `github`
2. Review the `dockerfile` in `/falcon/deployment/ubuntu18.04-falcon.Dockerfile`:
    1. If some`MPC` code is updated, change the docker file line 283-286
    2. Build locally with `cd deployment && docker build -t falcon-clean:latest -f ./ubuntu18.04-falcon.Dockerfile . --build-arg SSH_PRIVATE_KEY="$(cat ~/.ssh/id_rsa)" --build-arg CACHEBUST="$(date +%s)"`
3. Now the code is updated to the docker container.
4. Start the platform and submit the job according the following tutorials
5. If the `MPC` program is stable, then move the line 283-386 before the `ARG CACHEBUST=1` to avoid repeatly compile `MPC`

### Run the platform

1. Run coordinator with `bash 2022sigmod-exp/3party/coordinator/debug_coord.sh `

2. Run party server N with  `bash 2022sigmod-exp/3party/party0/debug_partyserver.sh --partyID N`

   e,g.  server 0 with `bash 2022sigmod-exp/3party/party0/debug_partyserver.sh --partyID 0`



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

## Run debug mode on the cluster

* create docker swarm service on the cluster

* source the go environment on the coordinator and each party server

```shell
export PATH=$PATH:${YOUR_GO_BIN_PATH}/go/bin
export GOROOT=${YOUR_GO_ROOT}/go
export GOPATH=${YOUR_GO_PATH}/gopath
export PATH=$GOROOT/bin:$GOPATH/bin:$PATH
export PATH=/root/.local/bin:$PATH
```

* configure the `COORD_SERVER_IP` and `COORD_SERVER_PORT` in `src/falcon_platform/config_coord.properties`, for example

```shell
JOB_DATABASE=sqlite3
COORD_SERVER_IP=10.0.0.20
COORD_SERVER_PORT=30004
# if COORD_SERVER_BASEPATH is not supplied, will default to LOG_PATH later in start_all script
COORD_SERVER_BASEPATH="/users/yunchengwu/projects/falcon/src/falcon_platform"
# number of consumers for the coord http server
N_CONSUMER=3
```

* start the coordinator by

```shell
cd src/falcon_platform
bash scripts/debug_coord.sh
```

* configure the docker swarm cluster for each party server, for example

```shell
COORD_SERVER_IP=10.0.0.20
COORD_SERVER_PORT=30004
PARTY_SERVER_IP=10.0.0.24
# Only used when deployment method is docker,
# When party server is a cluster with many servers, list all servers here,
# PARTY_SERVER_IP is the first element in PARTY_SERVER_CLUSTER_IPS
PARTY_SERVER_CLUSTER_IPS="10.0.0.24 10.0.0.32 10.0.0.33 10.0.0.34"
# label each node of cluster when launch the cluster,and list all node's label here, used to do schedule.
PARTY_SERVER_CLUSTER_LABEL="p2 p2w0 p2w1 p2w2"
# if PARTY_SERVER_BASEPATH must be supported, it should be the absolute path
PARTY_SERVER_BASEPATH="/users/yunchengwu/projects/falcon/src/falcon_platform"
# subprocess call paths
MPC_EXE_PATH="/opt/falcon/third_party/MP-SPDZ/semi-party.x"
FL_ENGINE_PATH="/opt/falcon/build/src/executor/falcon
```

* label the swarm nodes, for example

```shell
docker node update --label-add name=p0 [node-name]
```

* start each party server on the corresponding machine by

```shell
bash scripts/debug_partyserver.sh --partyID 0
```

* configure the parameters in the job json file, such as data path, if distributed, etc. submit the job by

```shell
python3 coordinator_client.py --url 10.0.0.20:30004 -method submit -path ./examples/train_job_dsls/three_parties_train_job_bank_distributed.json
```

* after submitting the job, can check the docker service is running correctly by

```shell
docker service ls 
bash scripts/docker_swarm_check.sh
```