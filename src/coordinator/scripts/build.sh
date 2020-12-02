

# build golang binary file
docker run --rm -v "$PWD":/usr/src/myapp -w /usr/src/myapp amd64/golang:1.14 make build