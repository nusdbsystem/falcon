#!/bin/bash

SERVER_LIST=aws_servers.txt

#docker save -o falcon.tar falcon:latest

while read REMOTE_SERVER
do
  ssh -i "/home/wuyuncheng/sigmod23-aws/aws-ec2-ssh-falcon.pem" -n -f $REMOTE_SERVER "docker pull lemonwyc/falcon:vldb23" &

done < $SERVER_LIST


