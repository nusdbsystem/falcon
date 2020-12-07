
. ./deploy/property/svc.properties

echo "Building platform image, used for coord, listener, master"
# build golang binary file
docker run --rm -v "$PWD":/usr/src/myapp -w /usr/src/myapp amd64/golang:1.14 make build_linux

# build docker image
docker build -t $FALCON_COORDINATOR_IMAGE  -f ./deploy/dockerfiles/platform.Dockerfile .

echo "Building platform image, used for train worker, prediction worker"

# build docker image
docker build -t $FALCON_WORKER_IMAGE  -f ./deploy/dockerfiles/executor.Dockerfile .