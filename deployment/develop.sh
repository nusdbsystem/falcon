

# push code to repo
git push origin
git push naili

# remove all containers
bash src/falcon_platform/scripts/docker_service_rm_all_container.sh

# build the docker file
cd deployment
docker build -t falcon-clean:latest -f ./ubuntu18.04-falcon.Dockerfile . --build-arg SSH_PRIVATE_KEY="$(cat ~/.ssh/id_rsa)" --build-arg CACHEBUST="$(date +%s)"

# submit the job
cd ../2022sigmod-exp
bash execute_dsls.sh 3party/DSLs/local_testing/new_feature/

