# for a source file in ./Programs/Source/, all SPDZ scripts must be run from ./.
# The setup-online.sh script must also be run from ./ to create the relevant data.

set -x
set -e

# Install third_party MP-SPDZ library
# cd /opt/falcon
cd third_party/MP-SPDZ
mv Math/Setup.h.prod Math/Setup.h
make -j 8 tldr

# set up online phase
# Scripts/setup-ssl.sh 3 (included in fast-make.sh)
bash fast-make.sh

# set up ssl
Scripts/setup-clients.sh 3

# compile high-level program
./compile.py Programs/Source/logistic_regression.mpc

sudo rm /usr/local/lib/libmpir*
sudo ln -s ./local/lib/libmpir* /usr/local/lib/
