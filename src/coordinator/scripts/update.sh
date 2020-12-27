


kubectl delete all --all

. config_coord.properties
rm -rf $BASE_PATH/database
rm -rf $BASE_PATH/run_time_logs/*
rm -rf $BASE_PATH/logs/*

. config_partyserver.properties
rm -rf $BASE_PATH/run_time_logs/*
rm -rf $BASE_PATH/logs/*

bash scripts/status.sh user

bash scripts/img_build.sh || exit 1

bash scripts/start_coord.sh || exit 1