#!/bin/bash

# 1. install docker
sudo snap install docker
sudo chmod 666 /var/run/docker.sock

# 2. dockerhub login
docker login
docker pull lemonwyc/falcon:latest

# 3. github pull code
git clone xxx
git checkout -b experiment2022 origin/experiment2022

# 2. join swarm cluster
docker swarm init --advertise-addr xxx.xxx.xxx.xxx
docker swarm join --token SWMTKN-1-1lwzl16598w94tklfitaloghny5u0wdk22ks6oxhriid30974i-6zjpnbmasrglioeoc99xq8dls 107.23.187.2:2377
docker node promote xxx
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

# 6. chmod nfs path
sudo chmod 777 /mnt/efs/fs1/