#!/bin/bash

. coordinator.properties

if [ $DATA_BASE_PATH ];then
	echo "DATA_BASE_PATH is exist, and echo to = $DATA_BASE_PATH"
else
	export DATA_BASE_PATH=$PWD
fi


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

  LOG_FILE_PATH=$DATA_BASE_PATH/logs/start_db.log

  (kubectl create configmap mysql-initdb-config --from-file=./deploy/property \
  &> $LOG_FILE_PATH) || exit 1

  bash ./scripts/create_mysql.sh $DATA_BASE_PATH || exit 1

  title "Starting falcon DB Done, Db are mounted at folder $DATA_BASE_PATH"

}

start_coordinator()
{
  echo "sdf"
}




title "Guidence"
help
#create_folders

title "Creating the cluster role binding"
# ensure python api in pod has auth to control kubernetes
kubectl create clusterrolebinding add-on-cluster-admin \
    --clusterrole=cluster-admin --serviceaccount=default:default

# Pull images from Docker Hub
# bash $HOST_WORKDIR_PATH/scripts/pull_images.sh || exit 1

if [ ! -n "$1" ] ;then
      # start all services
     start_db || exit 1
elif [[ $1 = "db" ]];then
     title "Launch Model: Start db"
     start_db || exit 1

elif [[ $1 = "admin" ]];then
     title "Launch Model: Start admin"
     start_admin || exit 1
else
    title "Unsupport arguments, please see the help doc"
fi

