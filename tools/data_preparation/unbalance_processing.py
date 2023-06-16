import numpy as np
import argparse
import csv


def get_args():
    parser = argparse.ArgumentParser(description='split the dataset into unbalanced or balanced datasets'
                                                 'according to labels and feature importance')

    parser.add_argument('--sample_data_path', type=str, help='the sampled dataset for interpretability')
    parser.add_argument('--sample_prediction_path', type=str, help='the predictions for interpretability')
    parser.add_argument('--sample_weight_path', type=str, help='the weights of sampled dataset')
    parser.add_argument('--num_clients', type=int, default=3, help='the number of clients in FL')
    parser.add_argument('--num_classes', type=int, default=2, help='the original dataset class number')
    parser.add_argument('--split_type', type=str, help='how to split the dataset w.r.t. feature importance'
                                                       'can only be balanced or unbalanced')
    parser.add_argument('--output_filename', type=str, help='the output file name')

    return parser.parse_args()


args = get_args()


def m(x, w):
    """Weighted Mean"""
    return np.sum(x * w) / np.sum(w)


def cov(x, y, w):
    """Weighted Covariance"""
    return np.sum(w * (x - m(x, w)) * (y - m(y, w))) / np.sum(w)


def corr(x, y, w):
    """Weighted Correlation"""
    return cov(x, y, w) / np.sqrt(cov(x, x, w) * cov(y, y, w))


# first step is to calculate the feature importance of sampled_data and predictions
# second step is to re-arrange the sampled_data indexes to balanced or unbalanced
# third step is to split and store the data by re-arranged indexes
sample_data = np.genfromtxt(args.sample_data_path, delimiter=',')
sample_weights = np.genfromtxt(args.sample_weight_path, delimiter=',')
sample_predictions = np.genfromtxt(args.sample_prediction_path, delimiter=',')
print("sample_data.shape = ", sample_data.shape)
print("sample_weights.shape = ", sample_weights.shape)
print("sample_predictions.shape = ", sample_predictions.shape)
if args.num_classes == 1:
    sample_prediction_vec = sample_predictions[:]
else:
    sample_prediction_vec = sample_predictions[:, 0]

feature_num = np.size(sample_data, 1)
print(sample_weights)
print(sample_prediction_vec)

pearson_coef_vec = []
for i in range(feature_num):
    pearson_coef_i = corr(sample_data[:, i], sample_prediction_vec, sample_weights)
    pearson_coef_vec.append(pearson_coef_i)
    print("pearson_coef_i = {:3.6f}".format(pearson_coef_i))

pearson_coef_vec_abs = []
for i in range(feature_num):
    pearson_coef_i_abs = abs(corr(sample_data[:, i], sample_prediction_vec, sample_weights))
    pearson_coef_vec_abs.append(pearson_coef_i_abs)
    print("pearson_coef_i_abs = {:3.6f}".format(pearson_coef_i_abs))


pearson_coef_sort_idx_asc = np.argsort(pearson_coef_vec_abs)
pearson_coef_sort_idx = pearson_coef_sort_idx_asc[::-1]
print(pearson_coef_sort_idx)

# calculate each client's dimension size
n_feature_per_client = int(feature_num / args.num_clients)
feature_sizes = []
n_feature_last_client = feature_num
for i in range(args.num_clients):
    if i < args.num_clients - 1:
        feature_sizes.append(n_feature_per_client)
        n_feature_last_client -= n_feature_per_client
    else:
        feature_sizes.append(n_feature_last_client)
print(feature_sizes)

# split according to type
split_sample_data = []

# for balanced setting, we split the important features evenly into the parties
if args.split_type == "balanced":
    print("balanced split")
    for i in range(args.num_clients):
        d = np.zeros((sample_data.shape[0], feature_sizes[i]))
        print("d.shape = ", d.shape)
        for j in range(n_feature_per_client):
            idx = pearson_coef_sort_idx[i + j * args.num_clients]
            print("j = %d, idx = %d", j, idx)
            d[:, j] = sample_data[:, idx]
        if i == args.num_clients - 1:
            # put the rest of features to the last client
            idx_start = n_feature_per_client * args.num_clients
            r = feature_num - idx_start
            for k in range(r):
                d[:, n_feature_per_client+k] = sample_data[:, pearson_coef_sort_idx[idx_start+k]]
        split_sample_data.append(d)

# for unbalanced setting, we let each party holds one important feature, and for the rest, we
# split them unevenly, which means party1: f1, f4, ... fa; party2: f2, f(a+1), ..., fb; party3: f3, f(b+1), ..., last
if args.split_type == "unbalanced":
    print("unbalanced split")
    argsort_cnt = 0
    for i in range(args.num_clients):
        d = np.zeros((sample_data.shape[0], feature_sizes[i]))
        print("d.shape = ", d.shape)
        for j in range(feature_sizes[i]):
            if j == 0:
                idx = pearson_coef_sort_idx[i]
            else:
                idx = pearson_coef_sort_idx[argsort_cnt+args.num_clients+j-1]
            print("j = %d, idx = %d", j, idx)
            d[:, j] = sample_data[:, idx]
        split_sample_data.append(d)
        argsort_cnt += (feature_sizes[i] - 1)

# for random setting, we let each party holds one important feature, and for the rest, we
# split them randomly, which means party1 randomly select a-1 features, etc.
if args.split_type == "random":
    print("random split")
    argsort_cnt = 0
    # extract the indexes from pearson_coef_sort_idx except the first three elements
    pearson_coef_sort_idx_extract = []
    for k in range(feature_num):
        if k > 2:
            pearson_coef_sort_idx_extract.append(pearson_coef_sort_idx[k])
    # permute pearson_coef_sort_idx_extract randomly
    np.random.shuffle(pearson_coef_sort_idx_extract)
    print(pearson_coef_sort_idx_extract)
    for k in range(feature_num-3):
        print("shuffled vec element = ", pearson_coef_sort_idx_extract[k])

    for i in range(args.num_clients):
        d = np.zeros((sample_data.shape[0], feature_sizes[i]))
        print("d.shape = ", d.shape)
        for j in range(feature_sizes[i]):
            if j == 0:
                idx = pearson_coef_sort_idx[i]
            else:
                idx = pearson_coef_sort_idx_extract[argsort_cnt + j - 1]
            print("j = %d, idx = %d", j, idx)
            d[:, j] = sample_data[:, idx]
        split_sample_data.append(d)
        argsort_cnt += (feature_sizes[i] - 1)


# write data to files
for i in range(args.num_clients):
    file_name_i = args.output_filename + str(i)
    np.savetxt(file_name_i, split_sample_data[i], delimiter=',', fmt='%.6f')