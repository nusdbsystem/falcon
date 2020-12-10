# Falcon Coordinator

## Dependencies for Development

- Server-side is Go (1.19 and above)
- k8s (V1.6)
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
   
   If in Linux:
   ```
   make build_linux
   ```
   
   If in MAC:
      ```
      make build_mac
      ```
   
   If in windows:
      ```
      make build_windows
      ```

1. Setup coordinator:
    
    Update config_coord.properties
    choose the MS_ENGINE, DATA_BASE_PATH, COORDINATOR_IP
    finally run script with following
    
    ```
    bash scripts/dev_start_coord.sh
    ```

2. Setup listener:
    
    Update config_listener.properties
    choose the LISTENER_IP, COORDINATOR_IP, LISTENER_NODE_PORT
    finally run script with following
    ```
    bash scripts/dev_start_listener.sh
    ```
   
## Platform setup production instruction

0. docker image is upload, if u wanna build yourself, try with:

   ```
   bash scripts/img_build.sh
   ```

1. Setup coordinator:
    Update config_coord.properties
    choose the MS_ENGINE, DATA_BASE_PATH, COORDINATOR_IP
    finally run script with following
    
    ```
    bash scripts/start_coord.sh
    ```

2. Setup listener:
    After coordinator running successfully, begin to run listener
    Update config_listener.properties
    choose the LISTENER_IP, COORDINATOR_IP, LISTENER_NODE_PORT
    finally run script with following

    ```
    bash scripts/start_listener.sh
    ```
      
## Use the platform

1. define your dsl.json, similar to the example provided in ./data/dsl.json

2. submit job:
    
    python coordinator_client.py -url <ip address of coordinator>:6573 -method submit -path ./data/dsl.json
    
    ```
    python coordinator_client.py -url 172.25.121.4:6573 -method submit -path ./data/dsl.json
    ```


3. kill job:
    
    python coordinator_client.py -url <ip address of coordinator>:6573 -method kill -job <job_id>
    
    ```
    python coordinator_client.py -url 172.25.121.4:6573 -method kill -job 60
    ```

4. query job status:
    
    python coordinator_client.py -url <ip address of coordinator>:6573 -method query_status -job <job_id>
    
    ```
    python coordinator_client.py -url 172.25.121.4:6573 -method query_status -job 60
    ```

## check the log

1.  log is at folder `$DATA_BASE_PATH/run_time_logs/` , 
    platform setup log is at `$DATA_BASE_PATH/logs/` ,
    db is at     `$DATA_BASE_PATH/database/` 
