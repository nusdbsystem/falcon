

. ./deploy/property/svc.properties


docker push $FALCON_COORD_IMAGE
docker push $FALCON_MASTER_IMAGE