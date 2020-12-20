![Alt text](https://github.com/lemonviv/falcon/blob/dev/src/coordinator/photos/db.png)

# Falcon Coordinator

## Dependencies for Development

- Server-side is Go (1.14 and above)
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


## Platform setup DEV instruction

0. Build Golang executable file
   


1. Setup coordinator:
    
    Update config_coord.properties
    choose the JOB_DATABASE, WORK_BASE_PATH, COORD_SERVER_IP
    finally run script with following
    
   If in Linux:
       ```
       bash scripts/dev_start_coord.sh build_linux
       ```
       
   If in MAC:
          ```
          bash scripts/dev_start_coord.sh build_mac
          ```
       
   If in windows:
          ```
          bash scripts/dev_start_coord.sh build_windows
          ```
          

2. Setup partyserver:
    
    Update config_partyserver.properties
    choose the PARTY_SERVER_IP, COORD_SERVER_IP, PARTY_SERVER_NODE_PORT
    finally run script with following
    
   If in Linux:
       ```
       bash scripts/dev_start_partyserver.sh build_linux
       ```
       
   If in MAC:
          ```
          bash scripts/dev_start_partyserver.sh build_mac
          ```
       
   If in windows:
          ```
          bash scripts/dev_start_partyserver.sh build_windows
          ```

## Platform setup production instruction

0. docker image is upload, if u wanna build yourself, try with:

   ```
   bash scripts/img_build.sh
   ```

1. Setup coordinator:
    Update config_coord.properties
    choose the JOB_DATABASE, WORK_BASE_PATH, COORD_SERVER_IP
    finally run script with following
    
    ```
    bash scripts/start_coord.sh
    ```

2. Setup partyserver:
    Update config_partyserver.properties
    choose the PARTY_SERVER_IP, COORD_SERVER_IP, PARTY_SERVER_NODE_PORT
    finally run script with following

    ```
    bash scripts/start_partyserver.sh
    ```
      
## Use the platform

1. define your job.json, similar to the example provided in ./data/job.json

2. submit job:
    
    python coordinator_client.py -url <ip url of coordinator>:6573 -method submit -path ./data/job.json
    
    ```
    python3 coordinator_client.py -url 127.0.0.1:30004 -method submit -path ./data/three_parties_train_job.json
    ```


3. kill job:
    
    python coordinator_client.py -url <ip url of coordinator>:6573 -method kill -job <job_id>
    
    ```
    python coordinator_client.py -url 172.25.123.254:30004 -method kill -job 60
    ```

4. query job status:
    
    python coordinator_client.py -url <ip url of coordinator>:6573 -method query_status -job <job_id>
    
    ```
    python coordinator_client.py -url 172.25.123.254:30004 -method query_status -job 60
    ```

## check the log

1.  log is at folder `$WORK_BASE_PATH/run_time_logs/` , 
    platform setup log is at `$WORK_BASE_PATH/logs/` ,
    db is at     `$WORK_BASE_PATH/database/` 
