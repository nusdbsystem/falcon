#!/bin/bash

. ./deploy/property/svc.properties

WORKER_NAME=$1
WORKER_TARGET_PORT=$2
MASTER_ADDR_PLACEHOLDER=$3
EXECUTOR_TYPE_PLACEHOLDER=$4
WORKER_ADDR_PLACEHOLDER=$5
SERVICE_NAME_PLACEHOLDER=$6
Env_PLACEHOLDER=$7
WORK_BASE_PATH=$8 # used to store logs

HOST_DATA_PATH=$9 # used to store train data
HOST_MODEL_PATH=${10}
HOST_DATA_OUTPUT=${11}

echo "$WORKER_ADDR_PLACEHOLDER"

BASE_PATH=$(echo "$WORK_BASE_PATH" | sed 's_/_\\/_g')

DATA_INPUT_PATH=$(echo "$HOST_DATA_PATH" | sed 's_/_\\/_g')
MODEL_PATH=$(echo "$HOST_MODEL_PATH" | sed 's_/_\\/_g')
DATA_OUTPUT_PATH=$(echo "$HOST_DATA_OUTPUT" | sed 's_/_\\/_g')

IMAGE=$(echo "$FALCON_WORKER_IMAGE" | sed 's_/_\\/_g')
# create new yaml according template
WORKER_YAML=./deploy/template/$WORKER_NAME.yaml
cp ./deploy/template/runtime_worker.yaml.template $WORKER_YAML || exit 1


# replace var in common yaml with customer defined variables
sed -i -e "s/WORKER_NAME/$WORKER_NAME/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_CLUSTER_PORT/$WORKER_TARGET_PORT/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_TARGET_PORT/$WORKER_TARGET_PORT/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_NODE_PORT/$WORKER_TARGET_PORT/g" $WORKER_YAML || exit 1
sed -i -e "s/FALCON_WORKER_IMAGE/$IMAGE/g" $WORKER_YAML || exit 1
sed -i -e "s/MASTER_ADDR_PLACEHOLDER/$MASTER_ADDR_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/SERVICE_NAME_PLACEHOLDER/$SERVICE_NAME_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/EXECUTOR_TYPE_PLACEHOLDER/$EXECUTOR_TYPE_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/WORKER_ADDR_PLACEHOLDER/$WORKER_ADDR_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/HOST_PATH/$BASE_PATH/g" $WORKER_YAML || exit 1
sed -i -e "s/Env_PLACEHOLDER/$Env_PLACEHOLDER/g" $WORKER_YAML || exit 1
sed -i -e "s/HOST_DATA_PATH/$DATA_INPUT_PATH/g" $WORKER_YAML || exit 1
sed -i -e "s/HOST_MODEL_PATH/$MODEL_PATH/g" $WORKER_YAML || exit 1
sed -i -e "s/HOST_DATA_OUTPUT/$DATA_OUTPUT_PATH/g" $WORKER_YAML || exit 1