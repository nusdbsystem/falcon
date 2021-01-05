set -e
set -x

cd /opt/falcon

cd third_party/libhcs
cmake .
make
sudo make install
