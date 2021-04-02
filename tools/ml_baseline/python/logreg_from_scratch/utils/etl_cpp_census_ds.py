"""
extract and load the coding-ai ml tutorial dataset
UCI adult census data
"""
# Common imports
# import csv
# import os
# import sys
# Scikit-Learn ≥0.20
from sklearn.model_selection import train_test_split
from sklearn import preprocessing
# numpy matplotlib
import numpy as np
# import matplotlib.pyplot as plt


def etl_cpp_census_ds():
    """
    data download from coding-ai repo
    https://github.com/coding-ai/machine_learning_cpp
    """
    with open('/opt/falcon/data/dataset/census_adult_data/adult_data.csv') as csv_file:
        dataset = np.loadtxt(csv_file, delimiter=",")

    print("census dataset shape, dtype = ",
          dataset.shape,
          dataset.dtype)

    # extract the first 16 columns as features X
    X = dataset[:, :-1]
    print("X.shape, X.dtype = ", X.shape, X.dtype)
    print("X[0] = ", X[0])

    # extract the last column as label y
    y = dataset[:, -1:].astype(np.int)
    # a 1d array was expected. Please change the shape of y to (n_samples, )
    y = y.ravel()
    print("y.shape, y[:5], y.dtype = ", y.shape, y[:5], y.dtype)
    print("=== X and y extraction done ===")

    # easier reading of standardized dataset
    np.set_printoptions(formatter={'float': lambda x: "{0:0.3f}".format(x)})

    # normalize with z-score standardization
    print("before normalization, X = ", X)       
    scaler = preprocessing.StandardScaler().fit(X)
    print("scaler.mean_ = ", scaler.mean_)
    print("scaler.scale_ = ", scaler.scale_)
    X = scaler.transform(X)
    print("After normalization, X = ", X)

    print("Data shape: " + str(X.shape))
    print("Labels shape: " + str(y.shape))

    X_train, X_test, y_train, y_test = train_test_split(
        X,
        y,
        test_size=0.2,
        random_state=42,
    )

    print("Train Data shape: " + str(X_train.shape))
    print("Train Labels shape: " + str(y_train.shape))
    print("Evaluation Data shape: " + str(X_test.shape))
    print("Evaluation Labels shape: " + str(y_test.shape))

    return X_train, X_test, y_train, y_test
