


serviceids=$(docker service ls -q)

mails=$(echo $serviceids | tr " " "\n")

for sid in $mails
do
  echo "$sid"
  STR=$(docker service ps $sid)
  SUB='Shutdown'
  if [[ "$STR" == *"$SUB"* ]]; then
    echo "$STR"
    echo
  fi
done

# show the loading process
ps aux | grep docker