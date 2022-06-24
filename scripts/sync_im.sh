#!/bin/bash

SERVER_LIST=servers.txt

#docker save -o falcon.tar falcon:latest

while read REMOTE_SERVER
do
  ssh -n -f $REMOTE_SERVER "docker load -i falcon.tar"

done < $SERVER_LIST


