"""
modified based on beckernick's implementation
blog link: https://beckernick.github.io/logistic-regression-from-scratch/
"""

import numpy as np


def sigmoid(scores):
    return 1 / (1 + np.exp(-scores))


def log_likelihood(features, target, weights):
    scores = np.dot(features, weights)
    # NOTE: divide by m samples
    # print("len(target) = ", len(target))
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
