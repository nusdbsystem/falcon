


container_id=$(docker run -i -t -d falcon:latest /bin/bash)

echo $container_id

docker cp src $container_id:/opt/falcon/
docker cp include $container_id:/opt/falcon/
# docker cp data $container_id:/opt/falcon/
# docker cp make.sh $container_id:/opt/falcon/
# docker cp deployment/docker_cmd.sh $container_id:/opt/falcon/

docker exec -it $container_id bash make.sh

docker container stop $container_id

docker commit --change='CMD ["bash", "docker_cmd.sh"]' $container_id falcon:latest

docker container rm $container_id

