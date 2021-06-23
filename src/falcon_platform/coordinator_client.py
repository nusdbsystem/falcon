import argparse
import requests
from pprint import pprint
import json


# for python scripts, this method is only for testing,
# the json body submit url is called by frontend
def submit_train_job_json():
    url = "http://127.0.0.1:30004/api/submit-train-job"

    jsonbody = {
  "job_name": "credit_card_training",
  "job_info": "this is the job_info",
  "job_fl_type": "vertical",
  "existing_key": 0,
  "party_nums": 2,
  "task_num": 1,
  "party_info": [
    {
      "id": 1,
      "addr": "127.0.0.1:30006",
      "party_type": "active",
      "path": {
        "data_input": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client0",
        "data_output": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client0",
        "model_path": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data"
      }
    },
    {
      "id": 2,
      "addr": "127.0.0.1:30007",
      "party_type": "passive",
      "path": {
        "data_input": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client1",
        "data_output": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data/client1",
        "model_path": "/home/wuyuncheng/Documents/falcon/data/dataset/bank_marketing_data"
      }
    }
  ],
  "tasks": {
    "pre_processing": {
      "mpc_algorithm_name": "preprocessing",
      "algorithm_name": "preprocessing",
      "input_configs": {
        "data_input": {
          "data": "file",
          "key": "key"
        },
        "algorithm_config": {
          "max_feature_bin": "32",
          "iv_threshold": "0.5"
        }
      },
      "output_configs": {
        "data_output": "outfile"
      }
    },
    "model_training": {
      "mpc_algorithm_name": "logistic_regression",
      "algorithm_name": "logistic_regression",
      "input_configs": {
        "data_input": {
          "data": "client.txt",
          "key": "mpcKeys"
        },
        "algorithm_config": {
          "batch_size": 32,
          "max_iteration": 50,
          "convergence_threshold": 0.0001,
          "with_regularization": 0,
          "alpha": 0.1,
          "learning_rate": 0.1,
          "decay": 0.1,
          "penalty": "l1",
          "optimizer": "sgd",
          "multi_class": "ovr",
          "metric": "acc",
          "differential_privacy_budget": 0.1
        }
      },
      "output_configs": {
        "trained_model": "model",
        "evaluation_report": "model.txt"
      }
    }
  }
}
    print("requesting to ", url)
    res = requests.post(url, json=jsonbody)
    print(res.status_code)
    try:
        pprint(json.loads(res.text))
    except:
        print(res.content)


def submit_train_job_file(url, path):
    url = "http://"+url+"/api/submit-train-job"
    with open(path, 'rb') as f:
        print("requesting to ", url)
        res = requests.post(url, files={'train-job-file': f})
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
        submit_train_job_file(args.url, args.path)

    if args.method == "submit_json":
        submit_train_job_json()

    if args.method == "kill":
        stop_train_job(args.url, args.job)

    if args.method == "query_status":
        query_train_job_status(args.url, args.job)
