"""
This program performs two different logistic regression implementations on two
different datasets of the format [float,float,boolean], one
implementation is in this file and one from the sklearn library. The program
then compares the two implementations for how well the can predict the given outcome
for each input tuple in the datasets.

@author Per Harald Borgen
"""
import numpy as np
import pandas as pd
# from pandas import DataFrame
from sklearn import preprocessing
# from sklearn.cross_validation import train_test_split
from sklearn.model_selection import train_test_split
from numpy import loadtxt, where
from matplotlib.pylab import scatter, show, legend, xlabel, ylabel


def etl_perborgen_toyds(visualize_data=False):
    # scale larger positive and values to between -1,1 depending on the largest
    # value in the data
    min_max_scaler = preprocessing.MinMaxScaler(feature_range=(-1,1))
    df = pd.read_csv("/opt/falcon/tools/ml_baseline/python/logreg_from_scratch/perborgen/data.csv", header=0)

    # clean up data
    df.columns = ["grade1","grade2","label"]

    x = df["label"].map(lambda x: float(x.rstrip(';')))

    # formats the input data into two arrays, one of independant variables
    # and one of the dependant variable
    X = df[["grade1","grade2"]]
    X = np.array(X)
    X = min_max_scaler.fit_transform(X)
    Y = df["label"].map(lambda x: float(x.rstrip(';')))
    Y = np.array(Y)


    # if want to create a new clean dataset
    ##X = pd.DataFrame.from_records(X,columns=['grade1','grade2'])
    ##X.insert(2,'label',Y)
    ##X.to_csv('data2.csv')

    # creating testing and training set
    X_train, X_test, Y_train, Y_test = train_test_split(
                                        X, Y,
                                        test_size=0.33,
                                        random_state=42)

    # visualize data, uncomment "show()" to run it
    if visualize_data:
        pos = where(Y == 1)
        neg = where(Y == 0)
        scatter(X[pos, 0], X[pos, 1], marker='o', c='b')
        scatter(X[neg, 0], X[neg, 1], marker='x', c='r')
        xlabel('Exam 1 score')
        ylabel('Exam 2 score')
        legend(['Not Admitted', 'Admitted'])
        show()

    return X_train, X_test, Y_train, Y_test
    # return X, X, Y, Y
