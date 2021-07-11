## Install protoc and protobuf

Prior to the inference gRPC pull request (https://github.com/lemonviv/falcon/pull/74/commits/16ef84f87afb4ae0523dcd8a04f361a7ea50660b), the protobuf/protoc version used is **3.0.0**.

After the inference gRPC pull request, the inference components in the system require gRPC of a **specific version** (such as 1.35.0 or 1.33.1) that **matches a particular protobuf version**.

The protobuf/protoc version required is now **3.14.0**.

_NOTE: if you use gRPC installed by vcpkg, the protobuf will be installed by vcpkg_

Below lists how you can install protobuf specific versions like `3.0.0` and `3.14.0`:
### How to Install Protobuf 3.0.0

From v3.0.0, a new language version (proto3)

Download the protobuf source from https://github.com/protocolbuffers/protobuf/releases/tag/v3.0.0

Get the zip `protobuf-cpp-3.0.0.tar.gz` (For example: if you only need C++, download `protobuf-cpp-[VERSION].tar.gz`; every package
contains C++ source already)

### How to Install Protobuf 3.14.0

Download `protobuf-cpp-[VERSION].tar.gz` for 3.14.0.

Download the protobuf source from (https://github.com/protocolbuffers/protobuf/releases/tag/v3.14.0)

Install to `/usr/local` by:

```sh
make clean
./configure
sudo echo
make && sudo make install && sudo ldconfig
```

Verify the installation:

```sh
$ which protoc
/usr/local/bin/protoc
$ protoc --version
libprotoc 3.14.0
```

### Installation Notes

From https://raw.githubusercontent.com/protocolbuffers/protobuf/master/src/README.md

C++ Installation - Unix
-----------------------

To build protobuf from source, the following tools are needed:

  * autoconf
  * automake
  * libtool
  * make
  * g++
  * unzip

On Ubuntu/Debian, you can install them with:

    $ sudo apt-get install autoconf automake libtool curl make g++ unzip


To build and install the C++ Protocol Buffer runtime and the Protocol
Buffer compiler (protoc) execute the following:


     ./configure
     make
     make check
     sudo make install
     sudo ldconfig # refresh shared library cache.

If "make check" fails, you can still install, but it is likely that
some features of this library will not work correctly on your system.
Proceed at your own risk.


By default, the package will be installed to **`/usr/local`**.

make sure to run `make clean` before building again.
