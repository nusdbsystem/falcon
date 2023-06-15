
#!/bin/bash

# exit on error
set -e
search_dir=./$1


for entry in "$search_dir"/*
do
  echo "Begin to process $entry"
  sed -i 's/,/ /g' $entry
done