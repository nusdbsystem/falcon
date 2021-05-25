bash scripts/stopall.sh || exit 1

bash scripts/img_build.sh || exit 1

bash scripts/start_coord.sh

#bash scripts/start_partyserver.sh

bash scripts/status.sh user
