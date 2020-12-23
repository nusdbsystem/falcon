# from base path
# cd src/coordinator/

# mkdir for relevant resources
rm -r dev_test
mkdir dev_test
cd dev_test

# setup coord folder
# rm -r coord
mkdir coord

# setup party 1
# rm -r party1
mkdir party1
cd party1
mkdir data_input
mkdir data_output
mkdir trained_model
mkdir logs

cd ../..
echo current path =
pwd

# launch coordinator FIRST!!
echo dev start coordinator
bash scripts/dev_start_coord.sh &
echo $! >> coord.pid

# then launch party 1
echo dev start party 1
bash scripts/dev_start_partyserver.sh &
echo $! >> party1.pid

# # then launch party 2
# echo dev start party 2
# bash scripts/dev_start_partyserver.sh &
# echo $! >> party2.pid

# test with python requests
# python3 coordinator_client.py -url 127.0.0.1:30004 -method submit -path ./data/single_party_train_job.json

# Kill the pid.pid with
# kill -9 $(cat pid.pid)
