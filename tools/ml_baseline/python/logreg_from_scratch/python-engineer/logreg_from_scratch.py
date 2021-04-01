import numpy as np


# follow step by step from tutorial:
# https://github.com/python-engineer/MLfromscratch/tree/master/mlfromscratch
class LogisticRegression:

    def __init__(self, learning_rate=0.001, n_iters=1000, thres=0.5):
        self.lr = learning_rate
        self.n_iters = n_iters
        self.weights = None
        self.bias = None
        self.thres = thres

    def fit(self, X, y):
        """
        take training samples X, and labels y
        X is numpy n-D vector, size mxn
            m = # samples
            n = # features
        y is a 1-D row vector, also of size m
        involve the training step and SGD
        """
        print("fit method called")
        # unpack mxn shape
        n_samples, n_features = X.shape
        print("n_samples, n_features = ", n_samples, n_features)

        # init parameters
        self.weights = np.zeros(n_features)
        self.bias = 0

        # gradient descent algorithm
        # iteratively update the weights
        for _ in range(self.n_iters):
            # print("--- {} iteration ---".format(_))
            # first apply the linear transformation
            # wx + b
            print("orginial weights = ", self.weights)
            print("original bias = ", self.bias)
            # approximate y with linear combination of weights and x, plus bias
            linear_model = np.dot(X, self.weights) + self.bias
            # apply sigmoid function
            # sigmoid function outputs the estimated prob
            y_predicted = self._sigmoid(linear_model)

            # compute gradients
            # update the weights with gradients
            # w = w - a dw
            # gradient of weights dw
            # sum of (x times difference(predicted_y - actual_y))
            # X.T transpose the mxn into nxm, to multiply the mx1 y
            dw = (1 / n_samples) * np.dot(X.T, (y_predicted - y))
            db = (1 / n_samples) * np.sum(y_predicted - y)
            print("dw = ", dw)
            print("db = ", db)
            # update parameters
            self.weights -= self.lr * dw
            self.bias -= self.lr * db

    def predict(self, X):
        linear_model = np.dot(X, self.weights) + self.bias
        y_predicted = self._sigmoid(linear_model)
        y_predicted_cls = [1 if i > 0.5 else 0 for i in y_predicted]
        return np.array(y_predicted_cls)

    def _sigmoid(self, x):
        """sigmoid function"""
        return 1 / (1 + np.exp(-x))
