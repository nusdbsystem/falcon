## Install and use GLog

GLog is the logging library from Google for C++. It makes it real easy for you to add logging to any C++ application.

### Install
Installing GLog header files and library files is easy:

`$ sudo apt install libgoogle-glog-dev`

### Header file

The header file to include in your source file is `glog/logging.h`

### Compile

To compile a source file using GLog, you will need to link using `-lglog`

### Reference
- https://codeyarns.com/tech/2017-10-26-how-to-install-and-use-glog.html


## Build Glog with Cmake from source

install glog from its latest source code and github:

Get the source code

`git clone https://github.com/google/glog.git`

Run CMake to configure the build tree

`cmake -H. -Bbuild -G "Unix Makefiles"`

Afterwards, generated files can be used to compile the project

`cmake --build build`


Test the build software (optional):

```bash
svd@svd-ThinkPad-T460:/opt/glog$ sudo cmake --build build --target test
[sudo] password for svd:   
Running tests...
Test project /opt/glog/build
      Start  1: demangle
 1/10 Test  #1: demangle .........................   Passed    0.00 sec
      Start  2: logging
 2/10 Test  #2: logging ..........................   Passed    1.04 sec
      Start  3: signalhandler
 3/10 Test  #3: signalhandler ....................   Passed    0.01 sec
      Start  4: stacktrace
 4/10 Test  #4: stacktrace .......................   Passed    0.00 sec
      Start  5: stl_logging
 5/10 Test  #5: stl_logging ......................   Passed    0.00 sec
      Start  6: symbolize
 6/10 Test  #6: symbolize ........................   Passed    0.00 sec
      Start  7: cmake_package_config_init
 7/10 Test  #7: cmake_package_config_init ........   Passed    0.03 sec
      Start  8: cmake_package_config_generate
 8/10 Test  #8: cmake_package_config_generate ....   Passed    1.87 sec
      Start  9: cmake_package_config_build
 9/10 Test  #9: cmake_package_config_build .......   Passed    0.66 sec
      Start 10: cmake_package_config_cleanup
10/10 Test #10: cmake_package_config_cleanup .....   Passed    0.01 sec

100% tests passed, 0 tests failed out of 10

Total Test time (real) =   3.64 sec
```
