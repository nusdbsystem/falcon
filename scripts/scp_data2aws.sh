#!/bin/bash

cd /home/wuyuncheng/sigmod23-aws/
scp -i "aws-ec2-ssh-falcon.pem" -r /opt/falcon/2022sigmod-exp/3party/party0/data/synthetic_lime/ ubuntu@ec2-107-23-187-2.compute-1.amazonaws.com:/mnt/efs/fs1/2022sigmod-exp/3party/party0/data/
scp -i "aws-ec2-ssh-falcon.pem" -r /opt/falcon/2022sigmod-exp/3party/party1/data/synthetic_lime/ ubuntu@ec2-107-23-187-2.compute-1.amazonaws.com:/mnt/efs/fs1/2022sigmod-exp/3party/party1/data/
scp -i "aws-ec2-ssh-falcon.pem" -r /opt/falcon/2022sigmod-exp/3party/party2/data/synthetic_lime/ ubuntu@ec2-107-23-187-2.compute-1.amazonaws.com:/mnt/efs/fs1/2022sigmod-exp/3party/party2/data/