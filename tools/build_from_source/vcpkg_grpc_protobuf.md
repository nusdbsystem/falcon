# Install gRPC and Protoc with vcpkg

Reference: https://stackoverflow.com/a/65110312
https://stackoverflow.com/questions/53805917/install-older-version-of-protobuf-via-vcpkg

Find the commit when gRPC and protoc is no longer compatible with falcon.

First for `protobuf`:
```bash
eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ git log --color=always --pretty='%Cred%h%Creset -%C(auto)%d%Creset %s %Cgreen(%ad)' --date=short | grep --color=never protobuf
7d472dd25 - [protobuf] Protobuf no longer compiles with vs2019 Update 16.10 w/ c++latest (#18251) (2021-06-10)
aefc4b01a - [protobuf] update to 3.15.8 (#16449) (2021-04-12)
4fb865b8b - [protobuf] Fix deprecation warning in vcpkg_check_feature() (#16997) (2021-04-02)
59c519f93 - [protobuf] Fix the default proto file path (#15246) (2020-12-28)
ad2933e97 - [protobuf] Update to 3.14.0 (#14670) (2020-12-01)
0b7e8c9e8 - [libprotobuf-mutator] Add new port (#13691) (2020-11-17)
```

Then for `grpc`:
```bash
eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ git log --color=always --pretty='%Cred%h%Creset -%C(auto)%d%Creset %s %Cgreen(%ad)' --date=short | grep --color=never grpc
79e72302c - [grpc] Update to v1.33.1 (#14187) (2020-10-28)
451491300 - [grpc] [arm-linux] Missing grpc plugins. (#14184) (2020-10-27)
```

## Switch to the commit and install

```bash
eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ git status
HEAD detached at 59c519f93

eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ ./vcpkg integrate install
Applied user-wide integration for this vcpkg root.

CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/home/eigen/bin/vcpkg/scripts/buildsystems/vcpkg.cmake"
eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ ./vcpkg install grpc
Computing installation plan...
The following packages will be built and installed:
    grpc[core]:x64-linux -> 1.33.1#1
  * protobuf[core]:x64-linux -> 3.14.0#1
Additional packages (*) will be modified to complete this operation.
```

## Compatible version of gRPC and protoc

```bash
eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ ./vcpkg list
abseil:x64-linux                                   2021-03-24#1     an open-source collection designed to augment th...
c-ares:x64-linux                                   1.17.1#1         A C library for asynchronous DNS requests
grpc:x64-linux                                     1.33.1#1         An RPC library and framework
openssl:x64-linux                                  1.1.1k#5         OpenSSL is an open source project that provides ...
protobuf:x64-linux                                 3.14.0#1         Protocol Buffers - Google's data interchange format
re2:x64-linux                                      2020-10-01       RE2 is a fast, safe, thread-friendly alternative...
upb:x64-linux                                      2020-12-19#1     μpb (often written 'upb') is a small protobuf i...
zlib:x64-linux                                     1.2.11#10        A compression library
```

## Uninstall and Remove

```bash
eigen@eigen-ThinkPad-T460:~/bin/vcpkg$ ./vcpkg remove protobuf --recurse
The following packages will be removed:
  * grpc:x64-linux
    protobuf:x64-linux
Additional packages (*) need to be removed to complete this operation.
Removing package grpc:x64-linux...
Removing package grpc:x64-linux... done
Removing package protobuf:x64-linux...
Removing package protobuf:x64-linux... done
```
