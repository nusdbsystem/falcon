


kubectl delete all --all

. config_coord.properties
rm -rf $COORD_SERVER_BASEPATH/database
rm -rf $COORD_SERVER_BASEPATH/runtime_logs/*
rm -rf $COORD_SERVER_BASEPATH/logs/*

. config_partyserver.properties
rm -rf $PARTY_SERVER_BASEPATH/runtime_logs/*
rm -rf $PARTY_SERVER_BASEPATH/logs/*

bash scripts/status.sh user

bash scripts/img_build.sh || exit 1

bash scripts/start_coord.sh || exit 1