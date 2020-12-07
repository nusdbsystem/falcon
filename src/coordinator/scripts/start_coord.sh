#!/bin/bash

. config_coord.properties


# load env variables
if [ $DATA_BASE_PATH ];then
	echo "DATA_BASE_PATH is exist, and echo to = $DATA_BASE_PATH"
else
	export DATA_BASE_PATH=$PWD
fi

# if using this scripts, assume running in production
env=prod

title()
{
    title="| $1 |"
    edge=$(echo "$title" | sed 's/./-/g')
    echo "$edge"
    echo "$title"
    echo "$edge"
}


help()
{
    cat <<- EOF
    Desc: used to launch the services
    Usage: start all services using: bash scripts/kubernetes/start.sh
           start db using: bash scripts/kubernetes/start.sh db
           start web using: bash scripts/kubernetes/start.sh web
EOF
}


start_db()
{
  title "Starting falcon DB..."

  bash ./scripts/_create_mysql.sh $DATA_BASE_PATH || exit 1

  title "Starting falcon DB Done, Db are mounted at folder $DATA_BASE_PATH"

}

start_redis()
{
  title "Starting falcon cache..."

  bash ./scripts/_create_redis.sh $DATA_BASE_PATH || exit 1

  title "Starting falcon redis Done, redis are mounted at folder $DATA_BASE_PATH"

}

start_coordinator()
{
  title "Starting falcon coord..."

  bash ./scripts/_create_coord.sh $DATA_BASE_PATH $env|| exit 1

  title "falcon coord started"

}

create_folders()
{
      title "Creating folders"
      mkdir $DATA_BASE_PATH
      mkdir $DATA_BASE_PATH/logs
      mkdir $DATA_BASE_PATH/run_time_logs

}

title "Guidence"
help
create_folders

title "Creating the cluster role binding"
# ensure python api in pod has auth to control kubernetes
kubectl create clusterrolebinding add-on-cluster-admin --clusterrole=cluster-admin --serviceaccount=default:default

# Pull images from Docker Hub
# bash $HOST_WORKDIR_PATH/scripts/pull_images.sh || exit 1

if [ ! -n "$1" ] ;then
      # start all services
     start_db || exit 1
     start_redis || exit 1
     start_coordinator || exit 1
elif [[ $1 = "db" ]];then
     title "Launch Model: Start db"
     start_db || exit 1

elif [[ $1 = "redis" ]];then
     title "Launch Model: Start redis"
     start_redis || exit 1

elif [[ $1 = "coord" ]];then
     title "Launch Model: Start coordinator"
     start_coordinator || exit 1
else
    title "Un-support arguments, please see the help doc"
fi

