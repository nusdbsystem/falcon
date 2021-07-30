# for a source file in ./Programs/Source/, all SPDZ scripts must be run from ./.
# The setup-online.sh script must also be run from ./ to create the relevant data.

set -x
set -e

# Install third_party MP-SPDZ library
cd /opt/falcon
cd third_party/MP-SPDZ
# get the latest commits
git pull origin master
# restore MP-SPDZ dir with git
git checkout Math/Setup.h.prod Math/Setup.h
git checkout CONFIG.mine
# (Optional) update the MP-SPDZ submodule
# git pull origin master

mv Math/Setup.h.prod Math/Setup.h

# below is needed for MPIR library
# If compiling MP-SPDZ for the first time, need to compile for mpir
# append "--fMPIR" flag
if [[ $* == *--fMPIR* ]]
then
    # try with fMPIR flag for mpir headers
    echo "Compiling for MPIR"
    # make -j 8 tldr line should be included for the first time we build the third-party library,
    # otherwise, the mpir dependency will not be compiled
    make -j 8 tldr
else
    # Default no flag in cmd
    # no need to build this part after May 11th 2021
    # But for the subsequent build on the same machine, this is not needed
    # as make tldr takes a long time
    echo "Skipping MPIR"
fi

# set up online phase
# Scripts/setup-ssl.sh 3 (included in fast-make.sh)
bash fast-make.sh

# set up ssl
Scripts/setup-clients.sh 3

# compile high-level program
./compile.py Programs/Source/logistic_regression.mpc

# grant permission for Player-Data folder (and all its contents)
sudo chmod -R 777 Player-Data/

sudo rm -f /usr/local/lib/libmpir*
# must specify the FULL PATH for symlink!!
sudo ln -s /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir* /usr/local/lib/
