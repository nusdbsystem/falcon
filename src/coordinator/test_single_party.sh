# from base path
cd src/coordinator/

# setup party 1
# mkdir for relevant resources
mkdir dev_test
cd dev_test
mkdir party1
cd party1
mkdir data_input
mkdir data_output
mkdir trained_model
mkdir logs

# setup coord folder
cd ..
mkdir coord

# launch coordinator FIRST!!
bash scripts/dev_start_coord.sh build_linux

# then launch party
bash scripts/dev_start_partyserver.sh build_linux
