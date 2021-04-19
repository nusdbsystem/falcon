#!/bin/bash

export PARTYSERVER_LOG_PATH=$1
env=$2

# load variables from properties
. config_partyserver.properties
. ./deploy/property/svc.properties

LOG_PATH=$(echo "$PARTYSERVER_LOG_PATH" | sed 's_/_\\/_g')
IMAGE=$(echo "$FALCON_COORD_IMAGE" | sed 's_/_\\/_g')


# create multi properties files
COMBINE_PROPERTIES=temp_combine.properties
awk -F= '!a[$1]++' ./deploy/property/svc.properties  ./config_partyserver.properties >  $COMBINE_PROPERTIES

# if using this scripts, assume running in production
echo 'Env='$env >> $COMBINE_PROPERTIES
echo 'SERVICE_NAME=partyserver' >> $COMBINE_PROPERTIES

# create common map, 当多次使用 --from-env-file 来从多个数据源创建 ConfigMap 时，仅仅最后一个 env 文件有效。
LOG_FILE_PATH=$PARTYSERVER_LOG_PATH/logs/start_partyserver.log
{
  (kubectl create configmap partyserver-config-$PARTY_ID --from-env-file=$COMBINE_PROPERTIES &> $LOG_FILE_PATH)
  echo "-------------------------- finish creating partyserver map for coordinator --------------------------------"
} || {
echo "--------------------------  creating config map error, check the log, --------------------------------"
}

# create new yaml according template
PARTYSERVER_YAML=./deploy/template/partyserver.yaml
cp ./deploy/template/partyserver.yaml.template $PARTYSERVER_YAML || exit 1

# replace var in common yaml with customer defined variables
sed -i -e "s/PARTYSERVER_PORT/$PARTY_SERVER_NODE_PORT/g" $PARTYSERVER_YAML || exit 1
sed -i -e "s/PARTYSERVER_TARGET_PORT/$PARTY_SERVER_NODE_PORT/g" $PARTYSERVER_YAML || exit 1
sed -i -e "s/PARTY_SERVER_NODE_PORT/$PARTY_SERVER_NODE_PORT/g" $PARTYSERVER_YAML || exit 1
sed -i -e "s/FALCON_COORD_IMAGE/$IMAGE/g" $PARTYSERVER_YAML || exit 1
sed -i -e "s/HOST_PATH/$LOG_PATH/g" $PARTYSERVER_YAML || exit 1
sed -i -e "s/PARTY_ID/$PARTY_ID/g" $PARTYSERVER_YAML || exit 1

# apply the job
kubectl apply -f $PARTYSERVER_YAML || exit 1
echo "-------------------------- finish creating partyserver, running at port $PARTY_SERVER_NODE_PORT ------------------------"

# delete common
rm -f $COMBINE_PROPERTIES
rm -f $PARTYSERVER_YAML
rm -f $PARTYSERVER_YAML"-e"
