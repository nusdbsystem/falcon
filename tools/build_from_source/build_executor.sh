set -e
set -x

cd /opt/falcon

rm -rf build/

mv src/executor/CMakeLists.txt.prod src/executor/CMakeLists.txt
mv test/CMakeLists.txt.prod test/CMakeLists.txt

mkdir build
cmake -Bbuild -H.

cd build/

make

# for rebuilding of executor
# just cd build/ && sudo make
