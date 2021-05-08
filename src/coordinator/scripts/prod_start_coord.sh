#!/bin/bash

. config_coord.properties


# load env variables
if [ $COORD_SERVER_BASEPATH ];then
	echo "COORD_SERVER_BASEPATH provided: $COORD_SERVER_BASEPATH"
else
	export COORD_SERVER_BASEPATH=$PWD
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

  bash ./scripts/_create_mysql.sh $COORD_SERVER_BASEPATH || exit 1

  title "Starting falcon DB Done, Db are mounted at folder $COORD_SERVER_BASEPATH"

}

start_redis()
{
  title "Starting falcon cache..."

  bash ./scripts/_create_redis.sh $COORD_SERVER_BASEPATH || exit 1

  title "Starting falcon redis Done, redis are mounted at folder $COORD_SERVER_BASEPATH"

}

start_coordinator()
{
  title "Starting falcon coord..."

  bash ./scripts/_create_coord.sh $COORD_SERVER_BASEPATH $env|| exit 1

  title "falcon coord started"

}

create_folders()
{
      title "Creating folders"
      mkdir $COORD_SERVER_BASEPATH
      mkdir $COORD_SERVER_BASEPATH/logs
      mkdir $COORD_SERVER_BASEPATH/runtime_logs

}

title "Guidence"
help
create_folders

title "Creating the cluster role binding"
# ensure python coordserver in pod has auth to control kubernetes
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
