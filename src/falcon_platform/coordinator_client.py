import argparse
import requests
from pprint import pprint
import json


def upload_train_job_file(url, path):
    url = "http://"+url+"/api/upload-train-job-file"
    with open(path, 'rb') as f:
        print("requesting to ", url)
        res = requests.post(url, files={'upload-train-job-file': f})
        print(res.status_code)
        try:
            pprint(json.loads(res.text))
        except:
            print(res.content)


def stop_train_job(url, jobId):
    url = "http://"+url+"/api/stop-train-job/"+jobId
    print("requesting to ", url)
    res = requests.get(url)
    print(res.status_code)
    try:
        pprint(json.loads(res.text))
    except:
        print(res.content)


def query_train_job_status(url, jobId):
    url = "http://"+url+"/api/query-train-job-status/"+jobId
    print("requesting to ", url)
    res = requests.get(url)
    print(res.status_code)
    try:
        pprint(json.loads(res.text))
    except:
        print(res.content)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-url', '--url', required=True, type=str, help="url")
    parser.add_argument('-method', '--method', required=True, type=str, help="method path")
    parser.add_argument('-path', '--path', required=False, type=str, help="file path")
    parser.add_argument('-job', '--job', required=False, type=str, help="job path")

    args = parser.parse_args()
    print(args.url, args.path, args.path)

    if args.method == "submit":
        upload_train_job_file(args.url, args.path)

    if args.method == "kill":
        stop_train_job(args.url, args.job)

    if args.method == "query_status":
        query_train_job_status(args.url, args.job)
