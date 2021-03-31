"""
References:
- https://ml-cheatsheet.readthedocs.io/en/latest/logistic_regression.html
- https://github.com/python-engineer/MLfromscratch/tree/master/mlfromscratch
"""

import numpy as np
import math_ops


def _estimate_prob(X, weights, bias, fit_bias=True):
    """
    X is the features

    weights is the paramters, theta

    fit_bias or fit_intercept, for whether to plus the
    constant _bias_ or _intercept_ term
    NOTE: the bias or intercept term can be incorporated into the
    weights matrix already, thus no need to add bias separately

    Returns the estimated probabiliy in range [0,1]
    """
    print("_estimate_prob method called")
    print("input X shape = ", X.shape)
    # first apply the linear transformation
    # wx + b
    # print(X.shape, self.weights.shape)
    # print(np.dot(X, self.weights))
    # print(self.bias)
    if fit_bias:
        linear_transformation = np.dot(X, weights) + bias
    else:
        linear_transformation = np.dot(X, weights)
    # then apply the logistic/sigmoid function
    # sigmoid function outputs the estimated prob
    est_prob = math_ops.sigmoid(linear_transformation)

    return est_prob


def predict(X, weights, bias, thres=0.5, fit_bias=True):
    """
    Returns 1D array of predicted labels
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


def cost_function(features, labels, weights):
    '''
    Using Mean Absolute Error

    Features:(100,3)
    Labels: (100,1)
    Weights:(3,1)
    Returns 1D matrix of predictions
    Cost = (labels*log(predictions) + (1-labels)*log(1-predictions) ) / len(labels)
    '''
    print("~~~cost_function called~~~")
    observations = len(labels)
    print("observatoins = ", observations)
#     print("weights = ", weights)

    predictions = predict(features, weights)
#     print("predictions = ", predictions)

    #Take the error when label=1
    # NOTE: if predictions == 0, then ln(0) = -inf
    # causing nan errors
    # use np.clip to clip min and max to near 0 or 1
    predictions = np.clip(predictions, a_min=0.00001, a_max=0.99999)
    class1_cost = -labels*np.log(predictions)
#     print("class1_cost = ", class1_cost)

    #Take the error when label=0
    class2_cost = (1-labels)*np.log(1-predictions)
#     print("class2_cost = ", class2_cost)

    #Take the sum of both costs
    cost = class1_cost - class2_cost
#     print("cost  = ", cost)

    #Take the average cost
    cost = cost.sum() / observations
    print("averaged cost = ", cost)

    return cost


def update_weights(features, labels, weights, lr):
    '''
    Vectorized Gradient Descent

    Features:(200, 3)
    Labels: (200, 1)
    Weights:(3, 1)
    '''
    N = len(features)

    #1 - Get Predictions
    predictions = predict(features, weights)

    #2 Transpose features from (200, 3) to (3, 200)
    # So we can multiply w the (200,1)  cost matrix.
    # Returns a (3,1) matrix holding 3 partial derivatives --
    # one for each feature -- representing the aggregate
    # slope of the cost function across all observations
    gradient = np.dot(features.T,  predictions - labels)

    #3 Take the average cost derivative for each feature
    gradient /= N

    #4 - Multiply the gradient by our learning rate
    gradient *= lr

    #5 - Subtract from our weights to minimize cost
    weights -= gradient

    return weights


def decision_boundary(prob, thres):
    return 1 if prob >= thres else 0


def classify(predictions):
    '''
    input  - N element array of predictions between 0 and 1
    output - N element array of 0s (False) and 1s (True)

    For e.g.
    Probabilities = [ 0.967, 0.448, 0.015, 0.780, 0.978, 0.004]
    Classifications = [1, 0, 0, 1, 1, 0]
    '''
    decision_boundary = np.vectorize(decision_boundary)
    return decision_boundary(predictions).flatten()


def train(features, labels, weights, lr, iters):
    """
    If our model is working, we should see our cost decrease after every iteration
    iter: 0 cost: 0.635
    iter: 1000 cost: 0.302
    iter: 2000 cost: 0.264
    """
    cost_history = []

    for i in range(iters):
        weights = update_weights(features, labels, weights, lr)

        #Calculate error for auditing purposes
        cost = cost_function(features, labels, weights)
        cost_history.append(cost)

        # Log Progress
        if i % 1 == 0:
            print("iter: "+str(i) + " cost: "+str(cost))

    return weights, cost_history


def accuracy(predicted_labels, actual_labels):
    diff = predicted_labels - actual_labels
    return 1.0 - (float(np.count_nonzero(diff)) / len(diff))

