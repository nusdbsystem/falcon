# for a source file in ./Programs/Source/, all SPDZ scripts must be run from ./.
# The setup-online.sh script must also be run from ./ to create the relevant data.

set -x
set -e

# Install third_party MP-SPDZ library
cd /opt/falcon
cd third_party/MP-SPDZ
# restore MP-SPDZ dir with git
git checkout Math/Setup.h.prod Math/Setup.h
git checkout CONFIG.mine

mv Math/Setup.h.prod Math/Setup.h
make -j 8 tldr

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
