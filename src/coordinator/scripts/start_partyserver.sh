#!/bin/bash

. config_partyserver.properties

# load env variables
if [ $WORK_BASE_PATH ];then
	echo "PARTYSERVER_BASE_PATH is exist, and echo to = $PARTYSERVER_BASE_PATH"
else
	export WORK_BASE_PATH=$PWD
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


start_partyserver()
{
  title "Starting falcon partyserver..."
  bash ./scripts/_create_partyserver.sh $WORK_BASE_PATH $env|| exit 1
  title "falcon coord partyserver"
}

create_folders()
{
      title "Creating folders"
      mkdir $WORK_BASE_PATH
      mkdir $WORK_BASE_PATH/logs
      mkdir $WORK_BASE_PATH/run_time_logs

}

title "Guidence"
help
create_folders

title "Creating the cluster role binding"
# ensure python coordserver in pod has auth to control kubernetes
kubectl create clusterrolebinding add-on-cluster-admin --clusterrole=cluster-admin --serviceaccount=default:default

# Pull images from Docker Hub
# bash $HOST_WORKDIR_PATH/scripts/pull_images.sh || exit 1

start_partyserver || exit 1
title "Creating PartyServer Done"
