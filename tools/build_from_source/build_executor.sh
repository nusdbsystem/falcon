set -e
set -x

cd /opt/falcon

rm -rf build/

# Before make, git checkout the CMkaeLists remants
git checkout src/executor/CMakeLists.txt
git checkout src/executor/CMakeLists.txt.prod
git checkout test/CMakeLists.txt
git checkout test/CMakeLists.txt.prod

mv src/executor/CMakeLists.txt.prod src/executor/CMakeLists.txt
mv test/CMakeLists.txt.prod test/CMakeLists.txt

mkdir build
cmake -Bbuild -H.

cd build/

make

# for rebuilding of executor
# just cd build/ && sudo make
