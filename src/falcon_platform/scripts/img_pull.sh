#!/bin/bash

source ./deploy/property/img.properties

docker pull $FALCON_COORD_IMAGE
docker pull $FALCON_WORKER_IMAGE