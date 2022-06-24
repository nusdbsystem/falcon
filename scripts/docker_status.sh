


serviceids=$(docker service ls -q)

mails=$(echo $serviceids | tr " " "\n")

for sid in $mails
do

  STR=$(docker service ps $sid)
  SUB='Running'
  if [[ "$STR" == *"$SUB"* ]]; then
    echo "$STR"
    echo
  fi
done
