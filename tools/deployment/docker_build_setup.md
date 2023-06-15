## Dockerfile and Docker Build

Try to build the docker image from `Dockerfile.ubuntu18.04`

Need to provide the `SSH_PRIVATE_KEY` build argument to the docker build command. The SSH key is need for the git submodules (private repos) in `third_party` directory:

```bash
# from tools/docker
docker build --network=host -t falcon:latest -f ./ubuntu18.04-falcon.Dockerfile . --build-arg SSH_PRIVATE_KEY="$(cat ~/.ssh/id_rsa)" --build-arg CACHEBUST="$(date +%s)"
```

Useful docker build flags:
- `-f`, file flag, for specifying the location of the dockerfile (By default, the Dockerfile is assumed to be located here, but you can specify a different location with the file flag (-f))
- `-t`, tag flag, for specifying a tag for the image `name:tag` format
- `--build-arg` for `ARG` in dockerfile

Run container and execute some cmd
```bash
docker run -i -t falcon:latest /bin/bash
```