import argparse
import requests


def submit_job(url, path):
    url = "http://"+url+"/submit"
    with open(path, 'rb') as f:
        print("requesting to ", url)
        res = requests.post(url, files={'dsl': f})
        print(res.status_code)
        try:
            print(res.json())
        except:
            pass


def kill_job(url, jobId):
    url = "http://"+url+"/stop"
    print("requesting to ", url)
    res = requests.post(url, data={'job_id': jobId})
    print(res.status_code)
    try:
        print(res.json())
    except:
        pass


def query_job_status(url, jobId):
    url = "http://"+url+"/query-job-status"
    print("requesting to ", url)
    res = requests.get(url, json={'job_id': jobId})
    print(res.status_code)
    try:
        print(res.content)
        print(res.json())
    except:
        pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-url', '--url', required=True, type=str, help="url")
    parser.add_argument('-method', '--method', required=True, type=str, help="method path")
    parser.add_argument('-path', '--path', required=False, type=str, help="file path")
    parser.add_argument('-job', '--job', required=False, type=str, help="job path")

    args = parser.parse_args()
    print(args.url, args.path, args.path)

    if args.method == "submit":
        submit_job(args.url, args.path)

    if args.method == "kill":
        kill_job(args.url, args.job)

    if args.method == "query_status":
        query_job_status(args.url, args.job)


'''
python coordinator_client.py -url 127.0.0.1:30004 -method submit -path /Users/nailixing/GOProj/src/github.com/falcon/src/coordinator/data/dsl.json
python coordinator_client.py -url 172.25.123.254:30004 -method kill -job 60
python coordinator_client.py -url 172.25.123.254:30004 -method query_status -job 60
'''
