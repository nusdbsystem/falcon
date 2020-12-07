#!/bin/bash

export DATA_BASE_PATH=$1
env=$2

# load variables from properties
. ./deploy/property/db.properties
. userdefined.properties
. ./deploy/property/svc.properties

BASE_PATH=$(echo "$DATA_BASE_PATH" | sed 's_/_\\/_g')
IMAGE=$(echo "$FALCON_COORDINATOR_IMAGE" | sed 's_/_\\/_g')


# create new yaml according template
COORD_STORAGE_YAML=./deploy/template/storage.yaml
cp ./deploy/template/storage.yaml.template $COORD_STORAGE_YAML || exit 1


# replace var in common yaml with customer defined variables
sed -i '' -e "s/STORAGE_NAME/$MASTER_STORAGE/g" $COORD_STORAGE_YAML || exit 1
sed -i '' -e "s/DATA_BASE_PATH/$BASE_PATH/g" $COORD_STORAGE_YAML || exit 1

# apply the job
kubectl apply -f $COORD_STORAGE_YAML || exit 1
echo "-------------------------- finish creating coordinator storage --------------------------------"

# create multi properties files
COMBINE_PROPERTIES=temp_combine.properties
awk -F= '!a[$1]++' ./deploy/property/db.properties ./deploy/property/svc.properties  ./userdefined.properties >  $COMBINE_PROPERTIES

# if using this scripts, assume running in production
echo 'Env='$env >> $COMBINE_PROPERTIES
echo 'SERVICE_NAME=coord' >> $COMBINE_PROPERTIES

# create common map, 当多次使用 --from-env-file 来从多个数据源创建 ConfigMap 时，仅仅最后一个 env 文件有效。
LOG_FILE_PATH=$DATA_BASE_PATH/logs/start_coord.log
{
  (kubectl create configmap coord-config --from-env-file=$COMBINE_PROPERTIES &> $LOG_FILE_PATH)
  echo "-------------------------- finish creating config map for coordinator --------------------------------"
} || {
echo "--------------------------  creating config map error, check the log, --------------------------------"
}


# create new yaml according template
COORD_YAML=./deploy/template/coordinator.yaml
cp ./deploy/template/coordinator.yaml.template $COORD_YAML || exit 1

# replace var in common yaml with customer defined variables
sed -i '' -e "s/MASTER_PORT/$MASTER_PORT/g" $COORD_YAML || exit 1
sed -i '' -e "s/MASTER_TARGET_PORT/$MASTER_TARGET_PORT/g" $COORD_YAML || exit 1
sed -i '' -e "s/MASTER_NODE_PORT/$MASTER_NODE_PORT/g" $COORD_YAML || exit 1
sed -i '' -e "s/FALCON_COORDINATOR_IMAGE/$IMAGE/g" $COORD_YAML || exit 1
sed -i '' -e "s/STORAGE_NAME/$MASTER_STORAGE/g" $COORD_YAML || exit 1

# apply the job
kubectl apply -f $COORD_YAML || exit 1
echo "-------------------------- finish creating coordinator --------------------------------"

# delete common
rm -f $COMBINE_PROPERTIES
rm -f $COORD_YAML
rm -f $COORD_STORAGE_YAML

