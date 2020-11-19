


# Falcon Coordinator

## Platform setup instruction
1. Setup coordinator:
    
    ./coordinator_server -cip <ip address>
    
    ```
    ./coordinator_server -cip 172.25.121.4
    ```


2. Setup listener:
    
    ./coordinator_server -svc listenser -cip <ip address of coordinator> -lip <ip address of listener>
    
    ```
    ./coordinator_server -svc listenser -cip 172.25.121.4 -lip 172.25.121.4
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

1. log is at folder .logs
