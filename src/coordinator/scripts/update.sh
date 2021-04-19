


kubectl delete all --all

. config_coord.properties
rm -rf $LOG_PATH/database
rm -rf $LOG_PATH/runtime_logs/*
rm -rf $LOG_PATH/logs/*

. config_partyserver.properties
rm -rf $LOG_PATH/runtime_logs/*
rm -rf $LOG_PATH/logs/*

bash scripts/status.sh user

bash scripts/img_build.sh || exit 1

bash scripts/start_coord.sh || exit 1