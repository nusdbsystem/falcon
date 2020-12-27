#!/bin/bash

. ./deploy/property/svc.properties


kubectl delete all --all
kubectl delete deploymenbat,svc mysql
kubectl delete pvc mysql-pv-claim
kubectl delete pv mysql-pv-volume
kubectl delete pvc $COORD_STORAGE-pvc
kubectl delete pv $COORD_STORAGE-pv
kubectl delete pvc $PARTYSERVER_STORAGE-pvc
kubectl delete pv $PARTYSERVER_STORAGE-pv

kubectl delete configmap mysql-initdb-config
kubectl delete configmap coord-config
kubectl delete configmap redis-config
kubectl delete configmap redis-envs
kubectl delete configmap partyserver-config
kubectl delete configmap partyserver-config-1
kubectl delete configmap partyserver-config-2

. config_coord.properties
rm -rf $BASE_PATH/database
rm -rf $BASE_PATH/runtime_logs/*
rm -rf $BASE_PATH/logs/*

. config_partyserver.properties

rm -rf /Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/.falcon_partyserver_1/*
rm -rf /Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/.falcon_partyserver_2/*

bash scripts/status.sh user
