#!/bin/bash

# 1. install docker
sudo snap install docker
sudo chmod 666 /var/run/docker.sock

# 2. dockerhub login
docker login
docker pull lemonwyc/falcon:latest

# 6. chmod nfs path
sudo chmod 777 /mnt/efs/fs1/

# 3. github pull code
git clone xxx
git checkout -b experiment2022 origin/experiment2022

# 2. join swarm cluster
docker swarm init --advertise-addr xxx.xxx.xxx.xxx
docker swarm join --token SWMTKN-1-47j1o757a9c55fegvbh2uj9ncyqhj7rzfis02hwxrwulpghc8j-bh5bbjhscpmx5qcchaxp0zroo 172.31.18.73:2377
docker swarm join --token SWMTKN-1-47j1o757a9c55fegvbh2uj9ncyqhj7rzfis02hwxrwulpghc8j-2ix6ygzu11742lzbo771fnmks 172.31.18.73:2377
docker node promote xxx
docker node demote xxx
docker node update --label-add name=p0w0 xxx
docker node ls -q | xargs docker node inspect -f '{{ .ID }} [{{ .Description.Hostname }}]: {{ .Spec.Labels }}'


# 3. install go and source env
wget -q https://golang.org/dl/go1.14.13.linux-amd64.tar.gz -O go114.tar.gz && tar xzf go114.tar.gz -C /home/ubuntu/
export GOROOT=/home/ubuntu/go
export GOPATH=/gopath
export PATH=$GOROOT/bin:$GOPATH/bin:$PATH
export PATH=/root/.local/bin:$PATH

# 4. install make
sudo apt install make

# 5. install gcc
sudo apt-get update && sudo apt install gcc

# run spdz on aws instance

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -h 172.31.18.73 -p 0 spdz_logistic_reg

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 2 -h 172.31.18.73 -p 0 spdz_mlp_2party

cp Player-Data/vary_party_num/6party/Input-P* Player-Data/

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 4 -h 172.31.18.73 -p 0 spdz_logistic_reg_4party

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 5 -h 172.31.18.73 -p 0 spdz_logistic_reg_5party

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 6 -h 172.31.18.73 -p 0 spdz_logistic_reg_6party

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -h 172.31.18.73 -p 0 spdz_logistic_reg_partyfeature10

cp Player-Data/vary_feature_num/partyfeature50/Input-P* Player-Data/

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -h 172.31.18.73 -p 0 spdz_mlp_partyfeature50

# run background on aws

nohup bash execute_dsls.sh 3party/DSLs/dsls_cent_lime/p1/ >> 20221214_run_batch1.out &