"""
modified based on beckernick's implementation
blog link: https://beckernick.github.io/logistic-regression-from-scratch/
"""

import numpy as np


def sigmoid(scores):
    return 1 / (1 + np.exp(-scores))


def log_likelihood(features, target, weights):
    """
    this cost function is derived by
    Section 4.4.1 of Hastie, Tibsharani, and Friedman’s
        Elements of Statistical Learning
    NOTE: the data need to be normalized/scaled
        for cost func to work, otherwise will encounter overfow in np.exp
    """
    # print("ll weights = ", weights)
    # print("ll features = ", features)
    scores = np.dot(features, weights)
    # print("ll scores = ", scores)
    # NOTE: divide by m samples
    # print("len(target) = ", len(target))
    # print("np.exp(scores) = ", np.exp(scores))
    # print("np.log(1 + np.exp(scores)) = ", np.log(1 + np.exp(scores)))
    # print("target*scores = ", target*scores)
    # print("np.sum(target*scores) = ", np.sum( target*scores))
    # print("np.sum( target*scores - np.log(1 + np.exp(scores)) ) = ", np.sum( target*scores - np.log(1 + np.exp(scores)) ))
    ll = np.sum( target*scores - np.log(1 + np.exp(scores)) )/len(target)
    return ll


def logistic_regression(features, target, num_steps, learning_rate, add_intercept = False):
    if add_intercept:
        intercept = np.ones((features.shape[0], 1))
        features = np.hstack((intercept, features))

    weights = np.zeros(features.shape[1])

    for step in range(num_steps):
        # print("\nstep ", step)
        # print("original weights = ", weights)
        scores = np.dot(features, weights)
        # print("scores = ", scores)
        predictions = sigmoid(scores)
        # print("predictions = ", predictions)

        # Update weights with gradient
        # print("target = ", target)
        output_error_signal = target - predictions
        # NOTE: divide by m samples
        gradient = np.dot(features.T, output_error_signal)/len(target)
        # print("gradient = ", gradient)
        weights += learning_rate * gradient

        # Print log-likelihood every so often
        if step % 10 == 0:
            print(log_likelihood(features, target, weights))

    return weights
