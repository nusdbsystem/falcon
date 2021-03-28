# clean up
rm -rf build

# create a build directory
mkdir build

# navigate to the build directory
# and run CMake to configure the project
# and generate a native build system
cd build
cmake ../

# Then call that build system to actually compile/link the project
cmake --build .

make
