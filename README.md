# Falcon

Falcon is a privacy-preserving and interpretable vertical federated learning system. It allows 
multiple parties to collaboratively train a variety of machine learning models, such as linear 
regression, logistic regression, decision tree, random forest, gradient boosting decision tree, 
and multi-layer perceptron models. The protection is achieved by a hybrid strategy of threshold 
partially homomorphic encryption (PHE) and additive secret sharing scheme (SSS), ensuring no 
intermediate information disclosure. Also, it supports prediction interpretability, which gives 
the parties an explanation on how the prediction is interpreted. Besides, it supports efficient data 
parallelism of the tasks to reduce the execution time.

## Prerequisites

* Ubuntu 18.04 

* install partially homomorphic encryption library **libhcs**.

* install secure multiparty computation library **MP-SPDZ**.

* install web server request handling library **[served](https://github.com/meltwater/served)**.

Note: if you are using docker image to run the examples, it is not needed to install these dependencies,
because they are already installed in the docker image. 

* install docker on the host machine.

## Run an example with Docker at local

### Build the docker image

Go to the `tools/deployment` folder, and run the following command to build the docker image:

```bash
docker build --network=host -t lemonwyc/falcon-pub:latest -f ./tools/deployment/ubuntu18.04-falcon.Dockerfile . --build-arg CACHEBUST="$(date +%s)"
```

It may take more than 1 hour to build this image as it needs to download a number of 
dependencies and build the MP-SPDZ programs for running the test examples. 

Alternatively, you can pull the built image and tag it with the name in each party's config `config_partyserver`:

```bash
docker pull lemonwyc/falcon-pub:Dec2023
docker tag lemonwyc/falcon-pub:Dec2023 falcon:latest
```

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
bash examples/3party/party1/debug_partyserver.sh --partyID 1
bash examples/3party/party2/debug_partyserver.sh --partyID 2
```

### Configure the docker swarm node label

Note that the label of the partyserver node is `PARTY_SERVER_CLUSTER_LABEL="host"` in the `examples/3party/coordinator/config_coord.properties`.
So, need to configure the corresponding label of `[NodeID]` in docker swarm, using the following commands.

```shell
docker node ls
docker node ls -q | xargs docker node inspect -f '{{ .ID }} [{{ .Description.Hostname }}]: {{ .Spec.Labels }}'
docker node update --label-add name=host [NodeID]
```

### Submit and run the job

Once the coordinator and three parties are started successfully, open another terminal for the user to submit 
a DSL job request. For example, train a logistic regression model on the breast_cancer dataset. Need to make sure
that the path in the example dsl `examples/3party/dsls/examples/train/8.train_logistic_reg.json` is correct.

```shell
cd examples/
python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path /opt/falcon/examples/3party/dsls/examples/train/8.train_logistic_reg.json
```

If the job is successfully submitted, it will return a job ID. You can query the status of this job using 
the `coordinator_client.py` script, go to the log folder to check the LOGs, or use `docker service ls` to 
view the containers. After the job is finished, can clean the docker containers using:

```shell
bash src/falcon_platform/scripts/docker_service_rm_all_container.sh
```

In case that the certificates for the MP-SPDZ library are expired, can enter the image container and re-generate 
the corresponding certificates as follows:

```shell
docker run --entrypoint /bin/bash -it falcon:latest
cd third_party/MP-SPDZ/
bash Scripts/setup-online.sh 3 128 128 && Scripts/setup-clients.sh 3 && Scripts/setup-ssl.sh 3 128 128 && c_rehash Player-Data/
cd /opt/falcon
bash make.sh
```

After that, open another terminal to commit the changes in the container to `falcon:latest` image by:
```shell
docker ps
docker commit --change='ENTRYPOINT ["bash", "deployment/docker_cmd.sh"]' [CONTAINER_ID] falcon:latest
```

Then, the rest of the steps are the same as the above.

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
2. Review the `dockerfile` in `/tools/deployment/ubuntu18.04-falcon.Dockerfile`:
    1. If some`MPC` code is updated, change the docker file line 283-286
    2. Build locally with `cd tools/deployment && docker build -t falcon:latest -f ./ubuntu18.04-falcon.Dockerfile . --build-arg SSH_PRIVATE_KEY="$(cat ~/.ssh/id_rsa)" --build-arg CACHEBUST="$(date +%s)"`
3. Now the code is updated to the docker container.
4. Start the platform and submit the job according the following tutorials
5. If the `MPC` program is stable, then move the line 283-386 before the `ARG CACHEBUST=1` to avoid repeatly compile `MPC`

### Run the platform

1. Run coordinator with `bash examples/3party/coordinator/debug_coord.sh `

2. Run party server N with  `bash examples/3party/party0/debug_partyserver.sh --partyID N`
   e,g.  server 0 with `bash examples/3party/party0/debug_partyserver.sh --partyID 0`

## Citation

If you use our code in your research, please kindly cite:
```
@article{DBLP:journals/pvldb/WuXCDLOXZ23,
  author    = {Yuncheng Wu and
               Naili Xing and
               Gang Chen and
               Tien Tuan Anh Dinh and
               Zhaojing Luo and
               Beng Chin OOi and
               Xiaokui Xiao and
               Meihui Zhang},
  title     = {Falcon: A Privacy-Preserving and Interpretable Vertical Federated Learning System},
  journal   = {Proc. {VLDB} Endow.},
  volume    = {16},
  number    = {10},
  pages     = {2471--2484},
  year      = {2023}
}
```

## Contact
To ask questions or report issues, please drop us an [email](mailto:lemonwyc@gmail.com).