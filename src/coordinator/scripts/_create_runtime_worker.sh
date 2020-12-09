#!/bin/bash

. ./deploy/property/svc.properties
. config_listener.properties

WORKER_NAME=$1
WORKER_TARGET_PORT=$2
MASTER_URL_PLACEHOLDER=$3
EXECUTOR_TYPE_PLACEHOLDER=$4
WORKER_URL_PLACEHOLDER=$5
SERVICE_NAME_PLACEHOLDER=$6
Env_PLACEHOLDER=$7

echo "$WORKER_URL_PLACEHOLDER"

BASE_PATH=$(echo "$DATA_BASE_PATH" | sed 's_/_\\/_g')

IMAGE=$(echo "$FALCON_COORDINATOR_IMAGE" | sed 's_/_\\/_g')
# create new yaml according template
WORKER_YAML=./deploy/template/$WORKER_NAME.yaml
cp ./deploy/template/runtime_worker.yaml.template $WORKER_YAML || exit 1


# replace var in common yaml with customer defined variables
sed -i -e "s/WORKER_NAME/$WORKER_NAME/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_CLUSTER_PORT/$WORKER_TARGET_PORT/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_TARGET_PORT/$WORKER_TARGET_PORT/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_NODE_PORT/$WORKER_TARGET_PORT/g" $WORKER_YAML || exit 1
sed -i -e "s/FALCON_WORKER_IMAGE/$IMAGE/g" $WORKER_YAML || exit 1
sed -i -e "s/MASTER_URL_PLACEHOLDER/$MASTER_URL_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/SERVICE_NAME_PLACEHOLDER/$SERVICE_NAME_PLACEHOLDER/g" $MASTER_YAML || exit 1
sed -i -e "s/EXECUTOR_TYPE_PLACEHOLDER/$EXECUTOR_TYPE_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_URL_PLACEHOLDER/$WORKER_URL_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/HOST_PATH/$BASE_PATH/g" $WORKER_YAML || exit 1
sed -i -e "s/Env_PLACEHOLDER/$Env_PLACEHOLDER/g" $WORKER_YAML || exit 1
