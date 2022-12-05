
#!/bin/bash

# exit on error
set -e

# go to falcon_platform dir
# cd /opt/falcon/2022sigmod-exp
# ./3party/DSLs

# execute all dsls in this dir.
search_dir=./$1
coord_ip=127.0.0.1:30004

echo "Executing all dsls under folder $search_dir"

for entry in "$search_dir"/*
do
  echo "Begin to submit dsl $entry"
  jobID=$(python3 coordinator_client.py --url $coord_ip -method submit -path $entry)

  echo "$entry is submitted, returned jobID=$jobID"
  sleep 2

  # keep waiting until previous job is finished.
  while : ; do
    output=$(python3 ./coordinator_client.py -url $coord_ip -method query_status -job $jobID)

    if [ "$output" == "finished" ]; then
      echo "output is $output, begin to execute next dsl."
      break
    elif [ "$output" == "failed" ]; then
      echo "output is $output, exit. "
      exit
    else
#      echo "output is $output, sleep two second, and waiting..."
      sleep 20
    fi
  done

  sleep 3
  echo "Now, $jobID finished, rm it's services and begin to execute the next dsl ..... "
  bash docker_rm_job.sh $jobID
  sleep 3

done

echo " All Job Done !"

