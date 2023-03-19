import numpy as np
import argparse
import csv
from sklearn.metrics import mean_squared_error


def get_args():
    parser = argparse.ArgumentParser(description='Calculate the accuracy of interpretability between plaintext'
                                                 'version and ciphertext version')

    parser.add_argument('--falcon_result_file', type=str, help='the interpretability result of falcon')
    parser.add_argument('--plaintext_result_file', type=str, help='the interpretability result of plaintext baseline')
    parser.add_argument('--num_classes', type=int, default=2, help='the number of classes to be explained')
    parser.add_argument('--eval_metric', type=str, default='mse', help='the metric to measure result distance')

    return parser.parse_args()


args = get_args()


# the result file will consist of the following lines
#   1st line: the sample weights of the instances;
#   num_classes * 2 lines: each class consists of 2 lines, one is pearson coefficients, the second is indexes
#   num_classes lines: each line denotes the final explanation coefficients

weight_mse = 0.0
pearson_mse = 0.0
explanation_mse = 0.0

falcon_res = []
plaintext_res = []
with open(args.falcon_result_file, "r") as falcon_csv:
    with open(args.plaintext_result_file, "r") as plaintext_csv:
        falcon_csv_reader = csv.reader(falcon_csv, delimiter=',')
        plaintext_csv_reader = csv.reader(plaintext_csv, delimiter=',')
        for falcon_row in falcon_csv_reader:
            falcon_res.append(falcon_row)
        for plaintext_row in plaintext_csv_reader:
            plaintext_res.append(plaintext_row)

# calculate the weight_mse
weight_mse = mean_squared_error(falcon_res[0], plaintext_res[0])

# calculate the pearson_mse
for i in range(args.num_classes):
    list1 = falcon_res[i*2+1]
    list2 = plaintext_res[i*2+1]
    pearson_mse = pearson_mse + mean_squared_error(list1, list2)
pearson_mse = pearson_mse / args.num_classes

# calculate the explanation_mse
for i in range(args.num_classes):
    idx = i + args.num_classes * 2 + 1
    list1 = falcon_res[idx]
    list2 = plaintext_res[idx]
    explanation_mse = explanation_mse + mean_squared_error(list1, list2)
explanation_mse = explanation_mse / args.num_classes

print("weight_mse = {:.5e}".format(weight_mse))
print("pearson_mse = {:.5e}".format(pearson_mse))
print("explanation_mse = {:.5e}".format(explanation_mse))