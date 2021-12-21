# checkout branch for test code
echo "---------------git checkout branch--------"
git fetch
#git checkout -b distributed-train-lr origin/distributed-train-lr

# pull latest code
echo "---------------git pull code--------"
git stash
git pull origin distributed-train-lr
git stash pop

# make executor
echo "---------------make executor--------"
bash make.sh

# make platform
echo "------Programs/Schedules/decision_tree.sch---------make platform--------"

cd ./src/falcon_platform

export GOROOT=/usr/local/go
export GOPATH=/gopath
export PATH=$GOROOT/bin:$GOPATH/bin:$PATH
export PATH=/root/.local/bin:$PATH

bash make_platform.sh Linux

echo "---------------run platform--------"
./bin/falcon_platform
