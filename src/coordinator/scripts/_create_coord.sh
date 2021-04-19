#!/bin/bash

export LOG_PATH=$1
env=$2

# load variables from properties
. ./deploy/property/db.properties
. config_coord.properties
. ./deploy/property/svc.properties

IMAGE=$(echo "$FALCON_COORD_IMAGE" | sed 's_/_\\/_g')
RUNTIMELOG_PATH=$(echo "$LOG_PATH"runtime_logs | sed 's_/_\\/_g')
echo $RUNTIMELOG_PATH
# create new yaml according template
COORD_STORAGE_YAML=./deploy/template/storage.yaml
cp ./deploy/template/storage.yaml.template $COORD_STORAGE_YAML || exit 1


# replace var in common yaml with customer defined variables
sed -i -e "s/STORAGE_NAME/$COORD_STORAGE/g" $COORD_STORAGE_YAML || exit 1
sed -i -e "s/RUNTIMELOG_PATH/$RUNTIMELOG_PATH/g" $COORD_STORAGE_YAML || exit 1

# apply the job
kubectl apply -f $COORD_STORAGE_YAML || exit 1
echo "-------------------------- finish creating coordinator storage --------------------------------"

# create multi properties files
COMBINE_PROPERTIES=temp_combine.properties
awk -F= '!a[$1]++' ./deploy/property/db.properties ./deploy/property/svc.properties  ./config_coord.properties >  $COMBINE_PROPERTIES

# if using this scripts, assume running in production
echo 'Env='$env >> $COMBINE_PROPERTIES
echo 'SERVICE_NAME=coord' >> $COMBINE_PROPERTIES

# create common map, 当多次使用 --from-env-file 来从多个数据源创建 ConfigMap 时，仅仅最后一个 env 文件有效。
LOG_FILE_PATH=$LOG_PATH/logs/start_coord.log
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
sed -i -e "s/COORD_CLUSTER_PORT/$COORD_CLUSTER_PORT/g" $COORD_YAML || exit 1
sed -i -e "s/COORD_TARGET_PORT/$COORD_TARGET_PORT/g" $COORD_YAML || exit 1
sed -i -e "s/COORD_NODE_PORT/$COORD_NODE_PORT/g" $COORD_YAML || exit 1
sed -i -e "s/FALCON_COORD_IMAGE/$IMAGE/g" $COORD_YAML || exit 1
sed -i -e "s/STORAGE_NAME/$COORD_STORAGE/g" $COORD_YAML || exit 1

# apply the job
kubectl apply -f $COORD_YAML || exit 1
echo "-------------------------- finish creating coordinator --------------------------------"

# delete common
rm -f $COMBINE_PROPERTIES
rm -f $COORD_YAML
rm -f $COORD_YAML"-e"
rm -f $COORD_STORAGE_YAML
rm -f $COORD_STORAGE_YAML"-e"

