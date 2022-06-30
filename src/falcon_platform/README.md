<!-- ![Alt text](https://github.com/lemonviv/falcon/blob/dev/src/falcon_platform/photos/db.png) -->
![Falcon Platform Architecture](../../imgs/falcon_platform/Falcon_Sys_Archi_Dec21version.jpg)

# Falcon Platform (Coordinator+PartyServers)

## Dependencies for Development

- Server-side is Go 1.14
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

## Supply the FL Engine and semi-party.x path etc

Update the executor path in `src/falcon_platform/config_partyserver.properties`: `FL_ENGINE_PATH="/opt/falcon/build/src/executor/falcon"`
Update the mpc path in `src/falcon_platform/config_partyserver.properties`: `MPC_EXE_PATH="/opt/falcon/third_party/MP-SPDZ/semi-party.x"`

Update configurations in
- `src/falcon_platform/config_coord.properties` for Coordinator server configs
- `src/falcon_platform/config_partyserver.properties` for Party server configs

Supply your configurations in those `.properties` files
## Platform setup Debug

**Simply call the `debug_coord.sh` script**:

```bash
# launch the coordinator 
bash scripts/debug_coord.sh
```

**Simply call the `debug_partyserver.sh` script to run few party servers **:

```bash
# launch the party server 1 
bash scripts/debug_partyserver.sh --partyID 1 
```

## Platform setup DEV ( without k8)


**Simply call the `dev_start_all.sh` script**:
```bash
# launch the coordinator and partyserver 0~(PARTY_COUNT-1)
bash scripts/dev_start_all.sh --partyCount <PARTY_COUNT>
```

To terminate the platform, call:
```bash
bash scripts/dev_terminate_all.sh --partyCount <PARTY_COUNT>
```


## Platform setup PROD ( with k8)

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

## UnitTest

To test all existing features, after building a cluster. 

1. Put all ready-to-be-tested DSL in a folder
2. Update the folder name in execute_dsls.sh
3. Go to the `falcon/src/falcon_platform` folder and run the following

```bash
bash scripts/execute_dsls.sh
```

## Interact with the platform (submit jobs, monitor jobs etc)

1. define your job.json, similar to the example provided in `./examples/train_job_dsls/job.json`

2. submit job:

   ```bash
   # python3 coordinator_client.py -url <ip url of coordinator>:30004 -method submit -path ./examples/train_job_dsls/job.json
   
   # UCI tele-marketing bank dataset
   python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path ./examples/full_template/8.train_logistic_reg.json
   # UCI breast cancer dataset
   python3 coordinator_client.py --url 127.0.0.1:30004 -method submit -path ./examples/full_template/three_parties_train_job_breastcancer_lr.json
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

## Interact with the platform from cmd line using curl (submit jobs)
```bash
curl -i http://127.0.0.1:30004/api/submit-train-job  \
    -H "Content-type: application/json" \
    -X POST \
    -d '{ "job_name": "credit_card_training", "job_info": "this is the job_info", "job_fl_type": "vertical", "existing_key": 0, "party_nums": 2, "task_num": 1, "party_info": [ { "id": 1, "addr": "127.0.0.1:30006", "party_type": "active", "path": { "data_input": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client0", "data_output": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client0", "model_path": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data" } }, { "id": 2, "addr": "127.0.0.1:30007", "party_type": "passive", "path": { "data_input": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client1", "data_output": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client1", "model_path": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data" } } ], "tasks": { "pre_processing": { "mpc_algorithm_name": "preprocessing", "algorithm_name": "preprocessing", "input_configs": { "data_input": { "data": "file", "key": "key" }, "algorithm_config": { "max_feature_bin": "32", "iv_threshold": "0.5" } }, "output_configs": { "data_output": "outfile" } }, "model_training": { "mpc_algorithm_name": "logistic_regression", "algorithm_name": "logistic_regression", "input_configs": { "data_input": { "data": "client.txt", "key": "mpcKeys" }, "algorithm_config": { "batch_size": 32, "max_iteration": 50, "convergence_threshold": 0.0001, "with_regularization": false, "alpha": 0.1, "learning_rate": 0.1, "decay": 0.1, "penalty": "l1", "optimizer": "sgd", "multi_class": "ovr", "metric": "acc", "differential_privacy_budget": 0.1, "fit_bias": true } }, "output_configs": { "trained_model": "model", "evaluation_report": "model.txt" } } } }'
```

```bash
curl -i http://127.0.0.1:30004/api/submit-train-job \
     -H "Content-Type: multipart/form-data" \
     -X POST \
     -F train-job-file=@'./falcon_platform/examples/train_job_dsls/debug_two_parties_train_job.json'
```

## check the log

1.  log is at folder `$LOG_PATH/runtime_logs/` , 
    platform setup log is at `$LOG_PATH/logs/` ,
    db is at     `falcon_logs/coord/falcon.db` 
    

The folder architecture is:
