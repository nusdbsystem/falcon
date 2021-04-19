#!/bin/bash

export LOG_PATH=$1

# load variables from properties
. ./deploy/property/db.properties
. config_coord.properties
. ./deploy/property/svc.properties

IMAGE=$(echo "$FALCON_COORD_IMAGE" | sed 's_/_\\/_g')
# create new yaml according template


# create new yaml according template
COORD_YAML=./deploy/template/coordinator.yaml
cp ./deploy/template/coordinator.yaml.template $COORD_YAML || exit 1

# replace var in common yaml with customer defined variables
sed -i -e "s/COORD_CLUSTER_PORT/$COORD_CLUSTER_PORT/g" $COORD_YAML || exit 1
sed -i -e "s/COORD_TARGET_PORT/$COORD_TARGET_PORT/g" $COORD_YAML || exit 1
sed -i -e "s/COORD_NODE_PORT/$COORD_NODE_PORT/g" $COORD_YAML || exit 1
sed -i -e "s/FALCON_COORD_IMAGE/$IMAGE/g" $COORD_YAML || exit 1
sed -i -e "s/STORAGE_NAME/$COORD_STORAGE/g" $COORD_YAML || exit 1

# apply the job
#kubectl replace --force -f $COORD_YAML || exit 1
kubectl apply -f $COORD_YAML || exit 1
echo "-------------------------- finish creating coordinator --------------------------------"

# delete common
rm -f $COORD_YAML
rm -f $COORD_YAML"-e"