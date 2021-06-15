<!-- ![Alt text](https://github.com/lemonviv/falcon/blob/dev/src/falcon_platform/photos/db.png) -->
![Falcon Platform Architecture](./imgs/Falcon_Sys_Archi_Dec21version.jpg)

# Falcon Platform (Coordinator+PartyServers)

## Dependencies for Development

- Server-side is Go (1.13 or 1.14)
- k8s (V1.9 best, V1.7 is also fine)
- Client-side is any http client, such as Python3 `requests`
- Storage default is file-based sqlite3, you can also connect to MySQL

**Go quick start**:
To run go code, you need go compiler

download the go compiler at https://golang.org/dl/

after the download, install by following the instruction at https://golang.org/doc/install

Verify that you've installed Go by opening a command prompt and typing the following command:
```sh
$ go version
```

to compile and run a go program
```sh
go run <go-program>.go
```

to compile into a binary
```sh
go build <go-program>.go
```

## Set up the MPC servers for FL

### Run `semi-party.x` from compiled MP-SPDZ in falcon/third_party

from `src/falcon_platform`, launch the script:

```sh
# start the 3 semi-party.x simulating 3 parties
bash scripts/start_semi-party012.sh
```

To view the MPC server outputs, go the path of where you put the MPC exe. In our example, the MPC server exes are in `MPC_EXE_PATH="/opt/falcon/third_party/MP-SPDZ/semi-party.x`. From that path, the console logs of the semi-party.x are captured in `third_party/MP-SPDZ/logs/semi-parties/` folder.

To terminate the MPC, from `src/falcon_platform`run:
```sh
bash scripts/terminate_semi-party012.sh
```

_NOTE: in the future, MPC and FL Engine will be launched internally by the platform after supplying `MPC_EXE_PATH="/opt/falcon/third_party/MP-SPDZ/semi-party.x"` in the config partyserver file_



## Supply the FL Engine path

Update the executor path in `src/falcon_platform/config_partyserver.properties`: `FL_ENGINE_PATH="/opt/falcon/build/src/executor/falcon"`


## Platform setup DEV (development without k8)

Update configurations in
- `src/falcon_platform/config_coord.properties` for Coordinator server configs
- `src/falcon_platform/config_partyserver.properties` for Party server configs

Supply your configurations in those `.properties` files such as
- `JOB_DATABASE`
- `LOG_PATH`
- `COORD_SERVER_IP`
- `PARTY_SERVER_IP`
- ...


**Simply call the `dev_start_all.sh` script**:
```bash
# launch the coordinator and partyserver 0~(PARTY_COUNT-1)
bash scripts/dev_start_all.sh --partyCount <PARTY_COUNT>
```

To terminate the platform, call:
```bash
bash scripts/dev_terminate_all.sh --partyCount <PARTY_COUNT>
```

The console outputs are captured in `src/falcon_platform/dev_test/` folder:
- `Coord_TIMESTAMP/Coord-console.log`
- `Party-0_TIMESTAMP/Party-0-console.log`
- `Party-1_TIMESTAMP/Party-1-console.log`
- `Party-2_TIMESTAMP/Party-2-console.log`


## Platform setup PROD (production with k8)

0. docker image is upload, if u wanna build yourself, try with:

   ```bash
   bash scripts/img_build.sh
   ```

1. Setup coordinator:
    Update config_coord.properties
    choose the JOB_DATABASE, LOG_PATH, COORD_SERVER_IP
    finally run script with following

    ```bash
    bash scripts/start_coord.sh
    ```

2. Setup partyserver:
    Update config_partyserver.properties
    choose the PARTY_SERVER_IP, COORD_SERVER_IP, PARTY_SERVER_NODE_PORT
    finally run script with following

    ```bash
    bash scripts/start_partyserver.sh
    ```

## Interact with the platform (submit jobs, monitor jobs etc)

1. define your job.json, similar to the example provided in `./train_jobs/job.json`

2. submit job:

    ```bash
    # python3 coordinator_client.py -url <ip url of coordinator>:30004 -method submit -path ./train_jobs/job.json
    # UCI tele-marketing bank dataset
    python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path ./train_jobs/two_parties_train_job.json
    # UCI breast cancer dataset
    python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path ./train_jobs/three_parties_train_job_breastcancer.json
    ```

3. kill job:

    ```bash
    # python3 coordinator_client.py -url <ip url of coordinator>:30004 -method kill -job <job_id>
    python3 coordinator_client.py -url 127.0.0.1:30004 -method kill -job 60
    ```

4. query job status:

    ```bash
    # python3 coordinator_client.py -url <ip url of coordinator>:30004 -method query_status -job <job_id>
    python3 coordinator_client.py -url 127.0.0.1:30004 -method query_status -job 60
    ```

## check the log

1.  log is at folder `$LOG_PATH/runtime_logs/` , 
    platform setup log is at `$LOG_PATH/logs/` ,
    db is at     `dev_test/coord/falcon.db` 

```bash
dev_test/
├── coord
│   ├── falcon.db
│   └── runtime_logs
│       └── coord2020-12-27T14:47:31.logs
├── party1
│   ├── data_input
│   ├── data_output
│   ├── logs
│   │   └── runtime_logs
│   │       └── partyserver2020-12-27T14:47:50.logs
│   └── trained_models
├── party2
│   ├── data_input
│   ├── data_output
│   ├── logs
│   │   └── runtime_logs
│   │       └── partyserver2020-12-27T14:47:45.logs
│   └── trained_models
└── party3
    ├── data_input
    ├── data_output
    ├── logs
    └── trained_models
...
```
