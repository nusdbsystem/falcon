#!/bin/bash

cd /opt/falcon/third_party/MP-SPDZ/

kill -9 $(cat ./logs/semi-parties/semi-party0.pid)
kill -9 $(cat ./logs/semi-parties/semi-party1.pid)
kill -9 $(cat ./logs/semi-parties/semi-party2.pid)

# just in case, kill after grep for keyword
# The grep filters that based on your search string,
# [x] is a trick to stop you picking up the actual grep process itself.
# ref: https://stackoverflow.com/a/3510850
kill $(ps aux | grep '[s]emi-party.x' | awk '{print $2}')

echo DONE
