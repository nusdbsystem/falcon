# Building from source

currently, The platform only can be built on <ubuntu:18.04>

## 0. Install package

```bash
apt-get update && apt-get upgrade -y && \
        apt-get install -y --no-install-recommends \
        git \
        sqlite3 \
        curl \
        sudo \
        unzip \
        vim \
        wget \
        zip \
        ssh
```


## 1. configure your ssh

Add your public key to your github "settings/SSH and GPG keys/SSH keys"


```bash
SSH_PRIVATE_KEY = "the path of your current .ssh/id_rsa"
mkdir /root/.ssh/
echo "${SSH_PRIVATE_KEY}" > /root/.ssh/id_rsa
chmod 600 /root/.ssh/id_rsa
```

Add host to known hosts

```bash
touch /root/.ssh/known_hosts
ssh-keyscan github.com >> /root/.ssh/known_hosts
```

## 2. Clone Falcon and init submodules

Default working dir is /opt
```bash
git clone git@github.com:lemonviv/falcon.git && \
    cd falcon/third_party/ && \
    git submodule update --init --recursive
```

## 3. install packages

```bash
apt-get update && apt-get upgrade -y && \
        apt-get install -y --no-install-recommends \
        libgmp-dev \
        libboost-dev \
        libboost-all-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libcurl4-openssl-dev \
        build-essential \
        ca-certificates \
        gdb \
        git \
        libjsoncpp-dev \
        liblog4cpp5-dev \
        libprotobuf-c0-dev \
        libprotobuf-dev \
        libssl-dev \
        libtool \
        m4 \
        protobuf-compiler \
        python \
        python3 \
        python3-pip \
        libgoogle-glog-dev \
        libgtest-dev \
        texinfo \
        yasm \
        automake \
        libsodium-dev \
        libcrypto++-dev \
        libcrypto++-doc \
        libcrypto++-utils \
        autoconf \
        pkg-config \
        wget \
        && \
        pip3 install requests && \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/*

dpkg-reconfigure dash
```


## 4. Upgrade cmake version to 3.19.7

```bash
mkdir ~/temp && \
    cd ~/temp && \
    wget https://cmake.org/files/v3.19/cmake-3.19.7.tar.gz && \
    tar -xzvf cmake-3.19.7.tar.gz && \
    cd cmake-3.19.7/ && \
    ./bootstrap && \
    make -j$(nproc) && \
    make install && \
    hash -r && \
    cmake --version
```


## 5. Install protobuf v3.14.0
```bash
cd ~/temp && \
    pwd && \
    wget https://github.com/protocolbuffers/protobuf/releases/download/v3.14.0/protobuf-all-3.14.0.tar.gz && \
    tar -xzvf protobuf-all-3.14.0.tar.gz && \
    cd protobuf-3.14.0 && \
    ./configure && \
    make && \
    make install && \
    ldconfig && \
    protoc --version && \
    cd cmake && \ 
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install
```


## 6. Install glog library

```bash
cd /opt
git clone https://github.com/google/glog.git && \
    cd glog && \
    cmake -H. -Bbuild -G "Unix Makefiles" && \
    cmake --build build
```

## 7. Ln gtest library

```bash
cd /usr/src/googletest/googletest && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    cp libgtest* /usr/lib/ && \
    cd .. && \
    rm -rf build && \
    mkdir /usr/local/lib/googletest && \
    ln -s /usr/lib/libgtest.a /usr/local/lib/googletest/libgtest.a && \
    ln -s /usr/lib/libgtest_main.a /usr/local/lib/googletest/libgtest_main.a
```

## 8. Install third_party threshold partially homomorphic encryption library

```bash

cd /opt/falcon
cd third_party/libhcs && \
    cmake . && \
    make && \
    make install
```

## 9. Install third_party MP-SPDZ library

```bash
cd /opt/falcon
cd third_party/MP-SPDZ && \
    mv Math/Setup.h.prod Math/Setup.h && \
    make -j 8 tldr && \
    bash fast-make.sh && \
    Scripts/setup-clients.sh 3 && \
    ./compile.py Programs/Source/logistic_regression.mpc && \
    ln -s /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir* /usr/local/lib/
```

## 10. Install served library

```bash
cd /opt/falcon
cd third_party/served && \
    mkdir cmake.build && \
    cd cmake.build && \
    cmake ../ && \
    make && \
    make install
```

## 11. Install Go 1.14

```bash
cd /opt/falcon
wget -q https://golang.org/dl/go1.14.13.linux-amd64.tar.gz -O go114.tar.gz && \
    tar xzf go114.tar.gz -C /usr/local
export PATH=$PATH:/usr/local/go/bin
```

## 12. Set environment variables.

```bash
export GOROOT=/usr/local/go
export GOPATH=/gopath
export PATH=$GOROOT/bin:$GOPATH/bin:$PATH
export PATH=/root/.local/bin:$PATH
```


## 13. Replace protoc version

```bash
cp ~/.local/bin/protoc /usr/bin/
```

## 14. Generate proto
```bash
cd /opt/falcon/src/executor/include/proto && \
    SRC_DIR=v0/ && \
    DST_DIR=../message && \
    protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto
```


## 15. build 
```bash
cd /opt/falcon
rm -rf build
export PATH="$PATH:$HOME/.local/bin" && \
    mkdir build && \
    cmake -Bbuild -H. && \
    cd build/ && \
    make
```

## 16. back to working directory 
```bash
cd /opt
```
