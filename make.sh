
# remove build folder
rm -rf build

# generate new protoc
cp ~/.local/bin/protoc /usr/bin/ && \
  cd /opt/falcon/src/executor/include/proto && \
  SRC_DIR=v0/ && \
  DST_DIR=../message && \
  protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto

# build project
cd /opt/falcon
export PATH="$PATH:$HOME/.local/bin" && \
    mkdir build && \
    cmake -Bbuild -H. && \
    cd build/ && \
    make -j 4

# c_rehash certificate
cd /opt/falcon/third_party/MP-SPDZ
c_rehash Player-Data/