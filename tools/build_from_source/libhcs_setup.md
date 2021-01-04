## libhcs

libhcs is a C library implementing a number of partially homormophic encryption
schemes


### Dependencies

- GMP
- CMake

To obtain the needed requirements on Ubuntu 15.10, one may run the following
command:

    sudo apt-get install libgmp-dev cmake


### Installation

Assuming all dependencies are on your system, the following will work on a
typical linux system.

    git clone https://github.com/Tiehuis/libhcs.git
    cmake .
    make
    sudo make install # Will install to /usr/local by default


To uninstall all installed files, one can run the following command:

    sudo xargs rm < install_manifest.txt


### Compile

To run this example, we need only need to link against libhcs and libgmp:

    clang -o example example.c -lhcs -lgmp
    ./example
