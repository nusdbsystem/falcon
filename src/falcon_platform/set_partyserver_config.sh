#!/bin/bash
# This script copy the required workloads, request json, and request client.py to the clients

shell_path=$(cd "$(dirname "$0")";pwd)

while getopts "p:" opt
do
    case $opt in
            p)
                party_id=${OPTARG}
                ;;
            *)
                echo "Usage: ./set_partyserver_config.sh -p <party_id>"
            exit 1;;
    esac
done

# copy to replace the
cp cluster_run_configs/config_partyserver_$party_id.properties ./config_partyserver.properties
bash scripts/debug_partyserver.sh --partyID $party_id