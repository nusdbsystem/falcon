# Falcon

Falcon is a federated learning system with strong privacy protection. It allows
multiple parties to collaboratively train a variety of machine learning models, 
serve for new requests, and calculate the interpretability.

## Prerequisites

* Ubuntu 18.04

* install partially homomorphic encryption library **libhcs**.

* install secure multiparty computation library **MP-SPDZ**.

* install web server request handling library **[served](https://github.com/meltwater/served)**.


## Run an example with Docker at local

### Build the docker image

Go to the `deployment` folder, and run the following command to build the docker image:

```bash
docker build --network=host -t falcon:latest -f ./ubuntu18.04-falcon.Dockerfile . --build-arg SSH_PRIVATE_KEY="$(cat ~/.ssh/id_rsa)" --build-arg CACHEBUST="$(date +%s)"
```

It may take a long time to build this image as it needs to download a number of 
dependencies and build the MP-SPDZ programs for running the test examples. 

### Install Go and source environment

Install go and source environment for running the coordinator and partyservers later.

```shell
wget -q https://golang.org/dl/go1.14.13.linux-amd64.tar.gz -O go114.tar.gz && tar xzf go114.tar.gz -C /home/ubuntu/
export GOROOT=/home/ubuntu/go
export GOPATH=/gopath
export PATH=$GOROOT/bin:$GOPATH/bin:$PATH
export PATH=/root/.local/bin:$PATH
```

### Start coordinator and partyservers

After building the docker image, open four terminals (one is the coordinator, and the other three
are the partyservers), and run the following commands under the project path in the terminals, respectively. 
Make sure the path and image name in `examples/3party/coordinator/config_coord.properties` are correctly defined.

```shell
# start the coordinator
bash examples/3party/coordinator/debug_coord.sh

# start three parties
bash examples/3party/party0/debug_partyserver.sh --partyID 0
bash examples/3party/party0/debug_partyserver.sh --partyID 1
bash examples/3party/party0/debug_partyserver.sh --partyID 2
```

### Submit and run the job

Once the coordinator and three parties are started successfully, open another terminal for the user to submit 
a DSL job request. For example, train a logistic regression model on the breast_cancer dataset. Need to make sure
that the path in the example dsl `examples/3party/dsls/local_testing/full_template/8.train_logistic_reg.json` is correct.

```shell
cd examples/
python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path /opt/falcon/examples/3party/dsls/local_testing/full_template/8.train_logistic_reg.json
```

If the job is successfully submitted, it will return a job ID. You can query the status of this job using 
the `coordinator_client.py` script, go to the log folder to check the LOGs, or use `docker service ls` to 
view the containers. After the job finished, can clean the docker containers using:

```shell
bash src/falcon_platform/scripts/docker_service_rm_all_container.sh
```

## Run examples on a distributed cluster 

### Setup Docker swarm service 

Install docker on three cluster nodes for three parties (the coordinator can be run on the active party).
And init a docker swarm service and add labels to the three nodes with `p0w0`, `p1w0`, `p2w0`, respectively.

```shell
sudo snap install docker
sudo chmod 666 /var/run/docker.sock

docker swarm init --advertise-addr xxx.xxx.xxx.xxx
docker swarm 
docker node promote xxx
docker node demote xxx
docker node update --label-add name=p0w0 xxx
docker node ls -q | xargs docker node inspect -f '{{ .ID }} [{{ .Description.Hostname }}]: {{ .Spec.Labels }}'

```

Follow the steps above to build a docker image and upload to DockerHub, and pull the image on the three nodes,
make sure each node has the corresponding image.

### Install Go and source environment

Follow the above steps to install go and source the environment.

### Update the config files

Go to the `examples/3party/` folder, update `config_coord.properties` file with the 
coordinator IP address `COORD_SERVER_IP=xxx.xxx.xxx.xxx`. 

Similarly, update the config file for  each partyserver under `examples/3party/party0/config_partyserver.properties`.

```shell
COORD_SERVER_IP=172.31.18.73
COORD_SERVER_PORT=30004
PARTY_SERVER_IP=172.31.18.73
PARTY_SERVER_NODE_PORT=30005
# Only used when deployment method is docker,
# When party server is a cluster with many servers, list all servers here,
# PARTY_SERVER_IP is the first element in PARTY_SERVER_CLUSTER_IPS
PARTY_SERVER_CLUSTER_IPS="172.31.18.73"
# 1. Label each node of cluster when launch the cluster,and list all node's label here, used to do schedule.
# 2. Label node with:: docker node update --label-add name=host j5eb3zmanmfd6wlgrby4qq101
# 3. Check label with:: docker node ls -q | xargs docker node inspect -f '{{ .ID }} [{{ .Description.Hostname }}]: {{ .Spec.Labels }}'
PARTY_SERVER_CLUSTER_LABEL="p0w0"
```

The `COORD_SERVER_IP` is the previous coordinator's IP address, and the `PARTY_SERVER_IP` is the current
node's IP address. Also, update the `PARTY_SERVER_CLUSTER_IPS` with all the available nodes for each party if 
running in the distributed mode. If each party only has one node, here should be the same as `PARTY_SERVER_IP`.
Besides, update the `PARTY_SERVER_CLUSTER_LABEL` with the label just created for the docker swarm node.

### Start coordinator and partyservres

Similarly, start one coordinator and three partyservers on the corresponding cluster node. Then, the 
coordinator is ready for accepting new job requests.

### Submit a job to run the test

Follow the above steps to submit a job on any cluster node, make sure to set `--url 127.0.0.1:30004` to
the coordinator IP address and port, for example `--url 172.31.18.73:30004`.


## Develop guide (for developers)

### Build the platform

Current development follows the current patterns:

1. Edit falcon or `MPC`  locally and `git commit` to corresponding `github`
2. Review the `dockerfile` in `/deployment/ubuntu18.04-falcon.Dockerfile`:
    1. If some`MPC` code is updated, change the docker file line 283-286
    2. Build locally with `cd deployment && docker build -t falcon-clean:latest -f ./ubuntu18.04-falcon.Dockerfile . --build-arg SSH_PRIVATE_KEY="$(cat ~/.ssh/id_rsa)" --build-arg CACHEBUST="$(date +%s)"`
3. Now the code is updated to the docker container.
4. Start the platform and submit the job according the following tutorials
5. If the `MPC` program is stable, then move the line 283-386 before the `ARG CACHEBUST=1` to avoid repeatly compile `MPC`

### Run the platform

1. Run coordinator with `bash examples/3party/coordinator/debug_coord.sh `

2. Run party server N with  `bash examples/3party/party0/debug_partyserver.sh --partyID N`

   e,g.  server 0 with `bash examples/3party/party0/debug_partyserver.sh --partyID 0`


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
COORD_SERVER_BASEPATH="/opt/falcon/src/falcon_platform"
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
PARTY_SERVER_BASEPATH="/opt/falcon/src/falcon_platform"
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