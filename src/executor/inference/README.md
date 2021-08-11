### Inference service

After starting the inference service, the server will listen on
a specified ip and port for handling new requests, for example,
`localhost:8123` is the default endpoint.

Then, we can start an HTTP request by the following format to 
send the inference requests:
```
curl 'http://localhost:8123/inference?sampleNum=3&sampleIds=1,2,3'
```
where the `sampleNum` denotes the number of samples to be inferred in
this request, and `sampleIds` denotes a set of sample ids, split by ','.