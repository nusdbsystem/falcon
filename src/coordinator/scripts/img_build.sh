
. ./deploy/property/img.properties


docker build -t $FALCON_COORDINATOR_IMAGE  -f ./deploy/dockerfiles/coordinator.Dockerfile .

