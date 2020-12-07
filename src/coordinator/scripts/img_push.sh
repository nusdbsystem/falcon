

. ./deploy/property/svc.properties


docker push $FALCON_COORDINATOR_IMAGE
docker push $FALCON_MASTER_IMAGE