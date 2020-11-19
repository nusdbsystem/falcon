


1. Setup coordinator:
    
    use ./coordinator_server -cip <ip address>
    for example: 
    
    ```
    ./coordinator_server -cip 172.25.121.4
    ```


2. Setup listener:
    
    use ./coordinator_server -svc listenser -cip <ip address of coordinator> -lip <ip address of listener>
    for example: 
    
    ```
    ./coordinator_server -svc listenser -cip 172.25.121.4 -lip 172.25.121.4
    ```

3. submit job:
    
    use: python coordinator_client.py -url <ip address of coordinator>:6573 -method submit -path ./data/dsl.json
    for example: 
    
    ```
    python coordinator_client.py -url 172.25.121.4:6573 -method submit -path ./data/dsl.json
    ```


4. kill job:
    
    use: python coordinator_client.py -url <ip address of coordinator>:6573 -method kill -job <job_id>
    for example: 
    
    ```
    python coordinator_client.py -url 172.25.121.4:6573 -method kill -job 60
    ```

5. query job status:
    
    use: python coordinator_client.py -url <ip address of coordinator>:6573 -method query_status -job <job_id>
    for example: 
    
    ```
    python coordinator_client.py -url 172.25.121.4:6573 -method query_status -job 60
    ```


