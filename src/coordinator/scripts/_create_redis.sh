#!/bin/bash

export DATA_BASE_PATH=$1

# load variables from properties
. ./deploy/property/db.properties
. ./deploy/property/svc.properties
. userdefined.properties

LOG_FILE_PATH=$DATA_BASE_PATH/logs/start_redis.log

{
(kubectl create configmap redis-config --from-file=./deploy/property/redis-config &> $LOG_FILE_PATH)
# error: from-env-file cannot be combined with from-file or from-literal
(kubectl create configmap redis-envs --from-env-file=./deploy/property/db.properties &> $LOG_FILE_PATH)
  echo "-------------------------- finish creating config map for redis --------------------------------"
}||{
  echo "-------------------------- creating config map error, check the log, --------------------------------"
}

# create new yaml according template
REDIS_YAML=./deploy/template/redis.yaml
cp ./deploy/template/redis.yaml.template $REDIS_YAML || exit 1


# replace var in common yaml with customer defined variables
sed -i '' -e "s/REDIS_SERVICE_NAME/$REDIS_HOST/g" $REDIS_YAML || exit 1
sed -i '' -e "s/REDIS_CLUSTER_PORT/$REDIS_CLUSTER_PORT/g" $REDIS_YAML || exit 1
sed -i '' -e "s/REDIS_TARGET_PORT/$REDIS_TARGET_PORT/g" $REDIS_YAML || exit 1
sed -i '' -e "s/REDIS_NODE_PORT/$REDIS_NODE_PORT/g" $REDIS_YAML || exit 1
sed -i '' -e "s/REDIS_IMAGE/$REDIS_IMAGE/g" $REDIS_YAML || exit 1

# apply the job
echo "--------------------------  creating cache service --------------------------------"
kubectl apply -f $REDIS_YAML &> $LOG_FILE_PATH|| exit 1

# delete common
rm -f $REDIS_YAML
