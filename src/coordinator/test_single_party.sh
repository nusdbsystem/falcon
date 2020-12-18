# from base path
#cd src/coordinator/ || exit 1

# setup party 1
# mkdir for relevant resources
mkdir .dev_test
cd .dev_test || exit 1
mkdir party1
cd party1 || exit 1
mkdir data_input
mkdir data_output
mkdir trained_model
mkdir logs

# setup coord folder
cd .. || exit 1
mkdir coord

# launch coordinator FIRST!!
#bash scripts/dev_start_coord.sh build_linux

# then launch party
#bash scripts/dev_start_partyserver.sh build_linux
