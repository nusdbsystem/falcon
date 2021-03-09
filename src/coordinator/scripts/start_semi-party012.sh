# export the MPC exe to PATH
# PATH=$PATH:/opt/falcon/third_party/MP-SPDZ/

cd /opt/falcon/third_party/MP-SPDZ/

# put the semi-party logs in logs/ if not already exists
OUT_DIR=logs/semi-parties/
mkdir -p $OUT_DIR

./semi-party.x -F -N 3 -p 0 -I logistic_regression > $OUT_DIR/semi-party0.log &
echo $! > $OUT_DIR/semi-party0.pid
./semi-party.x -F -N 3 -p 1 -I logistic_regression > $OUT_DIR/semi-party1.log &
echo $! > $OUT_DIR/semi-party1.pid
./semi-party.x -F -N 3 -p 2 -I logistic_regression > $OUT_DIR/semi-party2.log &
echo $! > $OUT_DIR/semi-party2.pid
