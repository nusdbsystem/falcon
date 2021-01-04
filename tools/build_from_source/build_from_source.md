## try building from source

**1. installed the apt packages**

such as:
- libgmp
- libboost
- libprotobuf
- libgoogle-glog-dev
- libgtest-dev
- libsodium
- libcrypto++


**2. install glog library (again?)**

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


**3. link gtest library (why creating the extra symlink?)**

create symbolic links using `ln` for `gtest` system locations

`ln -s [OPTIONS] FILE LINK`


If both the `FILE` and `LINK` are given, `ln` will create a link from the file specified as the first argument (FILE) to the file specified as the second argument (LINK)


**4. install third_party libhcs**

**5. install third_patry MP-SPDZ**

this step takes the most resources and time

**6. build falcon executor**

ran into problem when doing local build (during `make` step)

```bash
[ 58%] Building CXX object src/executor/CMakeFiles/executor.dir/operator/mpc/spdz_connector.cc.o
In file included from /home/svd/Documents/Work/NUS-SOC/Federated_Learning_with_Yuncheng_Jul2020/lemonviv-falcon/src/executor/operator/mpc/spdz_connector.cc:5:0:
/home/svd/Documents/Work/NUS-SOC/Federated_Learning_with_Yuncheng_Jul2020/lemonviv-falcon/include/falcon/operator/mpc/spdz_connector.h:9:10: fatal error: Math/gfp.h: No such file or directory
 #include "Math/gfp.h"
          ^~~~~~~~~~~~
compilation terminated.
src/executor/CMakeFiles/executor.dir/build.make:446: recipe for target 'src/executor/CMakeFiles/executor.dir/operator/mpc/spdz_connector.cc.o' failed
make[2]: *** [src/executor/CMakeFiles/executor.dir/operator/mpc/spdz_connector.cc.o] Error 1
```
