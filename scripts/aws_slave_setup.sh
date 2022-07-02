#!/bin/bash

# 1. install docker
sudo snap install docker
sleep 3
sudo chmod 666 /var/run/docker.sock

# 2. join swarm
docker swarm join --token SWMTKN-1-6a23vgnr9mu6v2k1xrx4dk1w3rlh0s1q30a03lqf2s67khi48b-7bat9ltrema6ddxg5zhxf24rk 172.31.17.197:2377

# 3. chmod nfs path
sudo chmod 777 /mnt/efs/fs1/

