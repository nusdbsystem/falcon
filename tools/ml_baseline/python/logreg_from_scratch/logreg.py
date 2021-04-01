"""
References:
- https://ml-cheatsheet.readthedocs.io/en/latest/logistic_regression.html
- https://github.com/python-engineer/MLfromscratch/tree/master/mlfromscratch
"""

import numpy as np
import math_ops


def _estimate_prob(X, weights, bias, fit_bias=True):
    """
    X is the features, m by n
    m = number of samples
    n = number of features
    X = Features:(m,n)

    weights is the paramters, theta
    if not including bias term, there are n model parameters
    Weights:(n,1)

    fit_bias or fit_intercept, for whether to plus the
    constant _bias_ or _intercept_ term
    NOTE: the bias or intercept term can be incorporated into the
    weights matrix already, thus no need to add bias separately

    Returns the estimated probabiliy in range [0,1]
    """
    print("_estimate_prob method called")
    # first apply the linear transformation
    # wx + b
    print("X.shape, weights.shape = ", X.shape, weights.shape)
    print("np.dot(X, weights) = ", np.dot(X, weights))
    print("bias = ", bias)
    if fit_bias:
        linear_transformation = np.dot(X, weights) + bias
    else:
        linear_transformation = np.dot(X, weights)
    print("linear_transformation = ", linear_transformation)
    # then apply the logistic/sigmoid function
    # sigmoid function outputs the estimated prob
    est_prob = math_ops.sigmoid(linear_transformation)
    print("est_prob = ", est_prob)

    return est_prob


def decision_boundary(prob, thres):
    """
    assign class labels (0 or 1) to predicted probabilities
    The threshold is used to make a decision.
    """
    return 1 if prob >= thres else 0


def predict(X, weights, bias, thres=0.5, fit_bias=True):
    """
    Returns 1D array of predicted labels 1 or 0
    """
    # computes the estimated prob
    est_prob = _estimate_prob(X, weights, bias, fit_bias=fit_bias)

    # discrete class based on the decision threshold
    y_predicted_cls = [decision_boundary(p, thres) for p in est_prob]

    return np.array(y_predicted_cls)


def predict_proba(X, weights, bias, fit_bias=True):
    """
    Returns 1D array of probabilities
    that the class label == 1
    """
    # computes the estimated prob
    est_prob = _estimate_prob(X, weights, bias, fit_bias=fit_bias)

    return np.array(est_prob)


def gradient_descent(X, y, weights, bias, lr, fit_bias=True):
    """
    m = number of samples
    n = number of features = number of weights

    X is Features: (m, n)
    y is true Labels (m, 1)
    Weights:(n, 1)
    bias term, scalar

    lr is learning rate

    fit_bias or fit_intercept, for whether to plus the
    constant _bias_ or _intercept_ term
    NOTE: the bias or intercept term can be incorporated into the
    weights matrix already, thus no need to add bias separately

    returns tuple the updated weights and bias
    (new_weights, new_bias)
    """
    # number of samples m
    # number of features n
    n_samples, n_features = X.shape

    print("original weights = ", weights)

    # Get Predictions
    y_predicted = predict_proba(X, weights, bias, fit_bias=fit_bias)
    print("y_predicted = ", y_predicted)
    print("y_predicted.shape = ", y_predicted.shape)
    print("y true = ", y)

    # update the weights with gradients
    # w = w - a dw
    # gradient of weights dw
    # sum of (x times difference(predicted_y - actual_y))
    # X.T transpose the (m,n) into (n,m), to multiply the (m,1) y
    # dw is (n,1) matrix holding n partial derivatives
    # Take the average cost derivative for each feature
    dw = (1/n_samples) * np.dot(X.T, y_predicted-y)
    if fit_bias:
        db = (1/n_samples) * np.sum(y_predicted-y)
    else:
        db = 0
    print("dw = ", dw)
    print("db = ", db)

    # gradient descent
    # Multiply the gradient by our learning rate
    # Subtract from our weights to minimize cost
    new_weights = weights - lr * dw
    print("new_weights = ", new_weights)
    new_bias = bias - lr * db
    print("new_bias = ", new_bias)

    return (new_weights, new_bias)
