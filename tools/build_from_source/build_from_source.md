# Building from source

## 1. installed the apt packages

such as:
- libgmp
- libboost
- libprotobuf
- libgoogle-glog-dev
- libgtest-dev
- libsodium
- libcrypto++


## 2. install glog library (with cmake)

The Google Logging Library (glog) implements application-level logging. The library provides logging APIs based on C++-style streams and various helper macros.

try first outside of `/opt`

```bash
local_build$ git clone https://github.com/google/glog.git && \
>     cd glog && \
>     cmake -H. -Bbuild -G "Unix Makefiles" && \
>     cmake --build build
```

test the glob installation with provided `cmake test`:

```bash
glog$ sudo cmake --build build --target test
```

for local outside of `/opt` build, ran into problem with IO permission at `.cmake` when `cmake -H. -Bbuild -G "Unix Makefiles"`:

```bash
CMake Warning at CMakeLists.txt:806 (export):
  Cannot create package registry file:

    /home/svd/.cmake/packages/glog/528d47ec07e94b6690cdb1e17fca9d09

  Permission denied
```

try using `sudo` with `cmake -H. -Bbuild -G "Unix Makefiles"`, got problem `-- Could NOT find Unwind (missing: Unwind_INCLUDE_DIR Unwind_LIBRARY Unwind_PLATFORM_LIBRARY)`

not sure if it is affecting the falcon platform...

cmake glog at `/opt` with `sudo` is fine


## 3. link gtest library (why creating the extra symlink?)

create symbolic links using `ln` for `gtest` system locations

`ln -s [OPTIONS] FILE LINK`


If both the `FILE` and `LINK` are given, `ln` will create a link from the file specified as the first argument (FILE) to the file specified as the second argument (LINK)

### Put Falcon Code Base at `/opt`

For sake of convenience, dock the falcon code base at `/opt`, and install `libhcs`, `mp-spdz`, and build `executor` from `/opt/falcon/`

## 4. install third_party libhcs

make sure you have the falcon code base at `/opt/falcon`

`sudo bash tools/build_from_source/libhcs_setup.sh`

## 5. install third_patry MP-SPDZ

this step takes the most resources and time

`sudo bash tools/build_from_source/mp-spdz_setup.sh`

_Need to grant permission for `Player-Data` folder_

### Check `/usr/local/lib`

After gtest, libhcs and MP-SPDZ installation, your `/usr/local/lib` should contain these files/links:

```
googletest/
libhcs.so*
libmpir.a -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir.a
libmpir.la -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir.la*
libmpir.so -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir.so*
libmpir.so.23 -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir.so.23*
libmpir.so.23.0.3 -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpir.so.23.0.3*
libmpirxx.a -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpirxx.a
libmpirxx.la -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpirxx.la*
libmpirxx.so -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpirxx.so*
libmpirxx.so.8 -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpirxx.so.8*
libmpirxx.so.8.4.3 -> /opt/falcon/third_party/MP-SPDZ/local/lib/libmpirxx.so.8.4.3*
```

## 6. Custom-install protobuf along with gRPC to be used by executor


### Install protobuf 3.14.0

The protoc version required for training used to be **3.0.0**, however, a version of **3.14.0** is required for the inference workflow.

Revise protobuf version to 3.14 that is compatible with grpc, follow the steps at [protobuf_setup](protobuf_setup.md).

### Install gRPC for C++ and CMake integration

_gRPC can be installed by either build from source, or via `vcpkg` package manager_.

#### build gRPC from source

Choose a directory to hold locally installed packages, add the directory to system `PATH`.

Follow the build from source from the gRPC.io website [grpc.io tutorial](https://grpc.io/docs/languages/cpp/quickstart/)

> The steps in the section explain now to build and locally install gRPC and Protocol Buffers using `cmake`.

#### install gRPC by `vcpkg`

found that gRPC can be installed by vcpkg [official link](https://github.com/grpc/grpc/tree/master/src/cpp#install-using-vcpkg-package)

> gRPC is available using the vcpkg dependency manager

In order to use `vcpkg` with CMake:
```sh
$ ./vcpkg integrate install
Applied user-wide integration for this vcpkg root.

CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/home/svd/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

Install `vcpkg` and `gRPC`:
```sh
vcpkg$ ./vcpkg install grpc
Computing installation plan...
The following packages will be built and installed:
  * abseil[core]:x64-linux -> 2020-09-23#2
  * c-ares[core]:x64-linux -> 1.17.1
    grpc[core]:x64-linux -> 1.33.1#2
  * openssl[core]:x64-linux -> 1.1.1j
  * protobuf[core]:x64-linux -> 3.14.0#1
  * re2[core]:x64-linux -> 2020-10-01
  * upb[core]:x64-linux -> 2020-08-19
  * zlib[core]:x64-linux -> 1.2.11#9
Additional packages (*) will be modified to complete this operation.
Detecting compiler hash for triplet x64-linux..
...
Starting package 1/8: abseil:x64-linux
Starting package 2/8: c-ares:x64-linux
Starting package 3/8: openssl:x64-linux
Starting package 4/8: protobuf:x64-linux
Starting package 5/8: re2:x64-linux
Starting package 6/8: upb:x64-linux
Starting package 7/8: zlib:x64-linux
Starting package 8/8: grpc:x64-linux

-- Installing: /home/svd/Downloads/vcpkg/packages/grpc_x64-linux/share/grpc/copyright
...
Total elapsed time: 14.79 min

The package grpc:x64-linux provides CMake targets:

    find_package(gRPC CONFIG REQUIRED)
    # Note: 7 target(s) were omitted.
    target_link_libraries(main PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)

    find_package(modules CONFIG REQUIRED)
    target_link_libraries(main PRIVATE re2::re2 c-ares::cares)
```

_NOTE: the gRPC version installed by `vcpkg` is `1.33.1`, to install latest gRPC, will need to contact the Microsoft maintainers on vcpkg_:

```sh
$ ./vcpkg search grpc
bond[bond-over-grpc]                  Bond-over-gRPC provides code generation from Bond IDL service definitions to s...
grpc                 1.33.1#2         An RPC library and framework
grpc[absl-sync]                       Use abseil synchronization module

If your library is not listed, please open an issue at and/or consider making a pull request:
    https://github.com/Microsoft/vcpkg/issues
```


## 7. build falcon executor

Run the build script from `tools/scripts/build_executor.sh`

_If the gRPC is installed via vcpkg, provide a flag `--vcpkg` to the above build script_: `bash tools/scripts/build_executor.sh --vcpkg`

**NOTE**: After edits of files in `src/executor`, simply re-make at `/opt/falcon/build`:
```sh
# for rebuilding of executor
# just cd build/ and
make
```

Verify that the `executor` is built correctly in `/opt/falcon/build/src/executor`:

```bash
# from /opt/falcon/build/src/executor
$ ls -lht
total 15M
-rwxr-xr-x 1 root root 8.4M Jan  5 15:03 falcon
-rw-r--r-- 1 root root 6.0M Jan  5 15:03 libexecutor.a
drwxr-xr-x 4 root root 4.0K Jan  5 14:30 CMakeFiles
-rw-r--r-- 1 root root 1.1K Jan  5 14:30 cmake_install.cmake
-rw-r--r-- 1 root root  31K Jan  5 14:30 Makefile
```

## Gotchas and FAQ

- Need to grant the dataset folder read/write permission

    for example, the dataset at `falcon/data/dataset/bank_marketing_data/` should be writable, otherwise the `phe_keys` files will be result in `Open file error` in executor step.

    `I0105 14:42:57.623278   786 io_util.cc:76] Open /opt/falcon/data/dataset/bank_marketing_data/client1/phe_keys file error.`

- Need to grant the `Player-Data` folder read/write permissions

    for example, the `C0.key` file after installing Mp-SPDZ is `-rw-------  1 root root`.

    The above `600` permission of `C0.key` will result in the executor error:

    ```
    terminate called after throwing an instance of 'std::runtime_error'
      what():  Cannot access /opt/falcon/third_party/MP-SPDZ/Player-Data/C0.key. Have you set up SSL?
    You can use `Scripts/setup-ssl.sh <nparties>`.
    ```


    In fact, the entire `Player-Data` needs to have the write permissions, otherwise the executor will report error:

    ```
    svd@svd-ThinkPad-T460:/opt/falcon/third_party/MP-SPDZ$ ./semi-party.x -F -N 3 -p 0 -I logistic_regression
    No modulus found in Player-Data//3-Dp-128/Params-Data, generating 128-bit prime
    Start listening on thread 139922300319488
    Party 0 is listening on port 14000 for external client connections.
    Listening for socket connections on base port 14000
    Starting a new iteration.
    Thread 139922300319488 found server.
    Party 0 received external client connection from client id: 2
    Thread 139922300319488 found server.
    Party 0 received external client connection from client id: 1
    Thread 139922300319488 found server.
    Party 0 received external client connection from client id: 0
    terminate called after throwing an instance of 'file_error'
      what():  File Error : IO problem when buffering gfp Triples from Player-Data//3-Dp-128/Triples-Dp-P0
    Aborted (core dumped)
    ```

- `compile.py` in MP-SPDZ warnings about order of memory warnings is ok

    ```
    /third_party/MP-SPDZ$ ./compile.py Programs/Source/logistic_regression.mpc
    WARNING: Order of memory instructions not preserved, errors possible
    ```

    the above warnings is not affecting the MPC program

- `Server-side handshake with C2 failed` due to certificates expire after a month.
  ```sh
  /opt/falcon/third_party/MP-SPDZ$ ./semi-party.x -F -N 3 -p 0 -I logistic_regression
  No modulus found in Player-Data//3-Dp-128/Params-Data, generating 128-bit prime
  Start listening on thread 140035952826112
  Party 0 is listening on port 14000 for external client connections.
  Listening for socket connections on base port 14000
  Starting a new iteration.
  Thread 140035952826112 found server.
  Server-side handshake with C2 failed. Make sure they have the necessary certificate (Player-Data/P0.pem in the default configuration), and run `c_rehash <directory>` on its location.
  Also make sure that it's still valid. Certificates generated with `Scripts/setup-ssl.sh` expire after a month.
  terminate called after throwing an instance of 'boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<boost::system::system_error> >'
    what():  handshake: stream truncated
  Aborted (core dumped)
  ```
  This is due to certificates from the build have expired, the default expiry when setting up the certifates is 1 month.
  
  **NOTE:** this is not related to whether to use "existing_key" in the train_job config. **this is key related to MPC, not the Falcon engine.**

  **Solution to this problem:** rebuild from `/tools/scripts/mp-spdz_setup.sh`

  **NOTE: `mv Math/Setup.h.prod Math/Setup.h` is critical! `*.prod` specifies `/opt/falcon` as base path**

