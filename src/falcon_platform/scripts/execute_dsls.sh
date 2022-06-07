
#!/bin/bash

# exit on error
set -e

# go to falcon_platform dir
# cd /opt/falcon/src/falcon_platform/

# execute all dsls in this dir.
search_dir=./examples/full_template/test_dsl
coord_ip=127.0.0.1:30004

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
    else
        echo "output is $output, sleep two second, and waiting..."
        sleep 2
    fi
  done


done

echo " All Job Done !"

