#!/bin/bash

. config_partyserver.properties

# load env variables
if [ $PARTY_SERVER_BASEPATH ];then
	echo "PARTY_SERVER_BASEPATH is exist, and echo to = $PARTY_SERVER_BASEPATH"
else
	export PARTY_SERVER_BASEPATH=$PWD
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
  bash ./scripts/_create_partyserver.sh $PARTY_SERVER_BASEPATH $env|| exit 1
  title "falcon coord partyserver"
}

create_folders()
{
      title "Creating folders"
      mkdir $PARTY_SERVER_BASEPATH
      mkdir $PARTY_SERVER_BASEPATH/logs
      mkdir $PARTY_SERVER_BASEPATH/runtime_logs

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
