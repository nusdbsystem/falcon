#!/bin/bash

export LISTENER_BASE_PATH=$1
env=$2

# load variables from properties
. config_listener.properties
. ./deploy/property/svc.properties

BASE_PATH=$(echo "$LISTENER_BASE_PATH" | sed 's_/_\\/_g')
IMAGE=$(echo "$FALCON_COORDINATOR_IMAGE" | sed 's_/_\\/_g')


# create multi properties files
COMBINE_PROPERTIES=temp_combine.properties
awk -F= '!a[$1]++' ./deploy/property/svc.properties  ./config_listener.properties >  $COMBINE_PROPERTIES

# if using this scripts, assume running in production
echo 'Env='$env >> $COMBINE_PROPERTIES
echo 'SERVICE_NAME=listener' >> $COMBINE_PROPERTIES

# create common map, 当多次使用 --from-env-file 来从多个数据源创建 ConfigMap 时，仅仅最后一个 env 文件有效。
LOG_FILE_PATH=$LISTENER_BASE_PATH/logs/start_listener.log
{
  (kubectl create configmap listener-config --from-env-file=$COMBINE_PROPERTIES &> $LOG_FILE_PATH)
  echo "-------------------------- finish creating listener map for coordinator --------------------------------"
} || {
echo "--------------------------  creating config map error, check the log, --------------------------------"
}

# create new yaml according template
LISTENER_YAML=./deploy/template/listener.yaml
cp ./deploy/template/listener.yaml.template $LISTENER_YAML || exit 1

# replace var in common yaml with customer defined variables
sed -i -e "s/LISTENER_PORT/$LISTENER_PORT/g" $LISTENER_YAML || exit 1
sed -i -e "s/LISTENER_TARGET_PORT/$LISTENER_TARGET_PORT/g" $LISTENER_YAML || exit 1
sed -i -e "s/LISTENER_NODE_PORT/$LISTENER_NODE_PORT/g" $LISTENER_YAML || exit 1
sed -i -e "s/FALCON_COORDINATOR_IMAGE/$IMAGE/g" $LISTENER_YAML || exit 1
sed -i -e "s/HOST_PATH/$BASE_PATH/g" $LISTENER_YAML || exit 1

# apply the job
kubectl apply -f $LISTENER_YAML || exit 1
echo "-------------------------- finish creating listener, running at port $LISTENER_NODE_PORT ------------------------"

# delete common
rm -f $COMBINE_PROPERTIES
rm -f $LISTENER_YAML
rm -f $LISTENER_YAML"-e"
