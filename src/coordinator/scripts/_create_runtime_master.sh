#!/bin/bash

. ./deploy/property/svc.properties

MASTER_NAME=$1
MASTER_TARGET_PORT=$2
ITEMKEY=$3
EXECUTOR_TYPE=$4
MASTER_URL_PLACEHOLDER=$5

echo "$MASTER_NAME"
echo "$MASTER_TARGET_PORT"
echo "$ITEMKEY"
echo "$EXECUTOR_TYPE"
echo "$MASTER_URL_PLACEHOLDER"

IMAGE=$(echo "$FALCON_COORDINATOR_IMAGE" | sed 's_/_\\/_g')

# create new yaml according template
MASTER_YAML=./deploy/template/$MASTER_NAME.yaml
cp ./deploy/template/runtime_master.yaml.template $MASTER_YAML || exit 1


# replace var in common yaml with customer defined variables

sed -i -e "s/MASTER_NAME/$MASTER_NAME/g" $MASTER_YAML || exit 1
sed -i -e "s/EXECUTOR_TYPE/$EXECUTOR_TYPE/g" $MASTER_YAML || exit 1
sed -i -e "s/MASTER_CLUSTER_PORT/$MASTER_TARGET_PORT/g" $MASTER_YAML || exit 1
sed -i -e "s/MASTER_TARGET_PORT/$MASTER_TARGET_PORT/g" $MASTER_YAML || exit 1
sed -i -e "s/MASTER_NODE_PORT/$MASTER_TARGET_PORT/g" $MASTER_YAML || exit 1
sed -i -e "s/FALCON_MASTER_IMAGE/$IMAGE/g" $MASTER_YAML || exit 1
sed -i -e "s/ITEMKEY/$ITEMKEY/g" $MASTER_YAML || exit 1
sed -i -e "s/COORD_SVC_NAME/$CoordSvcName/g" $MASTER_YAML || exit 1
sed -i -e "s/MASTER_URL_PLACEHOLDER/$MASTER_URL_PLACEHOLDER/g" $MASTER_YAML || exit 1

