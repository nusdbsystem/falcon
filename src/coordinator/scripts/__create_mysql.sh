#!/bin/bash

export DATA_BASE_PATH=$1

# load variables from properties
. ./deploy/property/db.properties
. coordinator.properties

# create new yaml according template
MYSQL_YAML=./deploy/template/mysql.yaml
cp ./deploy/template/mysql.yaml.template $MYSQL_YAML || exit 1

# create path variables
PV_PATH=$(echo $PV_DB_STORAGE_PATH | sed 's_/_\\/_g')

echo $PV_DB_STORAGE_PATH

# replace var in config yaml with customer defined variables
sed -i '' -e "s/MYSQL_CLUSTER_PORT/$MYSQL_CLUSTER_PORT/g" $MYSQL_YAML || exit 1
sed -i '' -e "s/PV_DB_STORAGE_PATH/$PV_PATH/g" $MYSQL_YAML || exit 1

# apply the job
kubectl apply -f $MYSQL_YAML || exit 1

# delete config
rm -f $MYSQL_YAML


