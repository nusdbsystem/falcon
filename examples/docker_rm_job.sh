

jobrm=$1

serviceids=$(docker service ls -q)

mails=$(echo $serviceids | tr " " "\n")

for sid in $mails
do

  STR=$(docker service ps $sid)
  if [[ "$STR" == *"job$jobrm"* ]]; then
    echo "$sid"
    docker service rm $sid
  fi
done

