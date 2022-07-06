


container_id=$(docker run -i -t -d lemonwyc/falcon:latest /bin/bash)

echo $container_id

docker cp src $container_id:/opt/falcon/
docker cp include $container_id:/opt/falcon/
# docker cp data $container_id:/opt/falcon/
# docker cp make.sh $container_id:/opt/falcon/
# docker cp deployment/docker_cmd.sh $container_id:/opt/falcon/

docker exec -it $container_id bash make.sh # && cd third_party/MP-SPDZ/ && bash Scripts/setup-clients.sh 3 && Scripts/setup-online.sh 3 128 128 && Scripts/setup-ssl.sh 3 128 128

docker container stop $container_id

docker commit --change='CMD ["bash", "docker_cmd.sh"]' $container_id lemonwyc/falcon:latest

docker container rm $container_id

bash Scripts/setup-online.sh 6 192 128

bash Scripts/setup-ssl.sh 6 192

