#!/bin/bash

. ./deploy/property/svc.properties

MASTER_NAME=$1
MASTER_TARGET_PORT=$2
ITEMKEY=$3
EXECUTOR_TYPE=$4

# create new yaml according template
MASTER_YAML=./deploy/template/$MASTER_NAME.yaml
cp ./deploy/template/master.yaml.template $MASTER_YAML || exit 1


# replace var in common yaml with customer defined variables
sed -i '' -e "s/MASTER_NAME/$MASTER_NAME/g" $MASTER_YAML || exit 1
sed -i '' -e "s/EXECUTOR_TYPE/$EXECUTOR_TYPE/g" $MASTER_YAML || exit 1
sed -i '' -e "s/MASTER_CLUSTER_PORT/$MASTER_TARGET_PORT/g" $MASTER_YAML || exit 1
sed -i '' -e "s/MASTER_TARGET_PORT/$MASTER_TARGET_PORT/g" $MASTER_YAML || exit 1
sed -i '' -e "s/MASTER_NODE_PORT/$MASTER_TARGET_PORT/g" $MASTER_YAML || exit 1
sed -i '' -e "s/FALCON_MASTER_IMAGE/$FALCON_MASTER_IMAGE/g" $MASTER_YAML || exit 1
sed -i '' -e "s/ITEMKEY/$ITEMKEY/g" $MASTER_YAML || exit 1