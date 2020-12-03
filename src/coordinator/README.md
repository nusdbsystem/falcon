# Falcon Coordinator

## Dependencies for Development

- Server-side is Go (1.14 and above)
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


## Platform setup instruction

0. Naming

   ```
   svc == service
   cip == coordinator IP
   lip == listener IP
   ```

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

1. log is at folder `logs`
