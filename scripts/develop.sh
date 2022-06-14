

container_id=$(docker run -i -t -d falcon-clean:latest /bin/bash)

echo $container_id

docker cp src $container_id:/opt/falcon/

docker exec -it $container_id bash make.sh

docker container stop $container_id


docker commit $container_id falcon-clean:latest


docker container rm $container_id






