

container_id=$(docker run -i -t -d falcon-clean:latest /bin/bash)

echo $container_id

docker cp src $container_id:/opt/falcon/

docker exec -it $container_id bash make.sh

docker container stop $container_id

docker commit --change='CMD ["bash", "docker_cmd.sh"]' $container_id falcon-clean

docker container rm $container_id






