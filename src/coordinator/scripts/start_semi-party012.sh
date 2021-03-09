# export the MPC exe to PATH
# PATH=$PATH:/opt/falcon/third_party/MP-SPDZ/

cd /opt/falcon/third_party/MP-SPDZ/

# put the semi-party logs in logs/ if not already exists
mkdir -p logs/semi-parties/

/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -p 0 -I logistic_regression > ./dev_test/semi-parties/semi-party0.log &
echo $! > ./logs/semi-parties/semi-party0.pid
/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -p 1 -I logistic_regression > ./dev_test/semi-parties/semi-party1.log &
echo $! > ./logs/semi-parties/semi-party1.pid
/opt/falcon/third_party/MP-SPDZ/semi-party.x -F -N 3 -p 2 -I logistic_regression > ./dev_test/semi-parties/semi-party2.log&
echo $! > ./logs/semi-parties/semi-party2.pid
