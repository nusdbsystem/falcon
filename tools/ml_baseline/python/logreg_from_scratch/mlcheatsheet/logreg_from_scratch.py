# step-by-step implementation of ml-cheatsheet tutorial
# https://ml-cheatsheet.readthedocs.io/en/latest/logistic_regression.html
import numpy as np
import matplotlib.pyplot as plt


"""
NOTE: mlcheatsheet implementation by default
does not consider bias or intercept term,
unless you reshape the features
and weights manually
"""


def sigmoid(z):
    """
    NOTE: require the data to be normalized,
    otherwise will produce:
        RuntimeWarning: overflow encountered in exp
    """
    return 1.0 / (1 + np.exp(-z))


def predict(features, weights):
    '''
    Returns 1D array of probabilities
    that the class label == 1
    '''
    z = np.dot(features, weights)
    return sigmoid(z)


def cost_function(features, labels, weights):
    '''
    Using Mean Absolute Error

    Features:(100,3)
    Labels: (100,1)
    Weights:(3,1)
    Returns 1D matrix of predictions
    Cost = (labels*log(predictions) + (1-labels)*log(1-predictions) ) / len(labels)
    '''
    # print("~~~cost_function called~~~")
    observations = len(labels)
    # print("observatoins = ", observations)
    # print("weights = ", weights)

    predictions = predict(features, weights)
    # print("predictions = ", predictions)

    # Take the error when label=1
    # NOTE: if predictions == 0, then ln(0) = -inf
    # causing nan errors
    # use np.clip to clip min and max to near 0 or 1
    predictions = np.clip(predictions, a_min=0.00001, a_max=0.99999)
    class1_cost = -labels*np.log(predictions)
    # print("class1_cost = ", class1_cost)

    # Take the error when label=0
    class2_cost = (1-labels)*np.log(1-predictions)
    # print("class2_cost = ", class2_cost)

    # Take the sum of both costs
    cost = class1_cost - class2_cost
    # print("cost  = ", cost)

    # Take the average cost
    cost = cost.sum() / observations
    # print("averaged cost = ", cost)

    return cost


def update_weights(features, labels, weights, lr):
    '''
    Vectorized Gradient Descent

    Features:(200, 3)
    Labels: (200, 1)
    Weights:(3, 1)
    '''
    N = len(features)

    # 1 - Get Predictions
    predictions = predict(features, weights)

    # 2 Transpose features from (200, 3) to (3, 200)
    # So we can multiply w the (200,1)  cost matrix.
    # Returns a (3,1) matrix holding 3 partial derivatives --
    # one for each feature -- representing the aggregate
    # slope of the cost function across all observations
    gradient = np.dot(features.T,  predictions - labels)

    # 3 Take the average cost derivative for each feature
    gradient /= N

    # 4 - Multiply the gradient by our learning rate
    gradient *= lr

    # 5 - Subtract from our weights to minimize cost
    weights -= gradient

    return weights


def classify(predictions):
    '''
    input  - N element array of predictions between 0 and 1
    output - N element array of 0s (False) and 1s (True)

    For e.g.
    Probabilities = [ 0.967, 0.448, 0.015, 0.780, 0.978, 0.004]
    Classifications = [1, 0, 0, 1, 1, 0]
    '''
    def decision_boundary(prob):
        return 1 if prob >= .5 else 0

    decision_boundary = np.vectorize(decision_boundary)
    return decision_boundary(predictions).flatten()


def train(features, labels, weights, lr, iters, print_every=10):
    """
    If our model is working, we should see our cost decrease after every iteration
    iter: 0 cost: 0.635
    iter: 1000 cost: 0.302
    iter: 2000 cost: 0.264
    """
    cost_history = []

    for i in range(iters):
        # print("\niter-", i)
        weights = update_weights(features, labels, weights, lr)

        # Calculate error for auditing purposes
        cost = cost_function(features, labels, weights)
        cost_history.append(cost)

        # Log Progress
        if i % print_every == 0:
            print("iter: "+str(i) + " cost: "+str(cost))

    return weights, cost_history


def accuracy(predicted_labels, actual_labels):
    """
    simply compare predicted labels to true labels and divide by the total.
    """
    diff = predicted_labels - actual_labels
    return 1.0 - (float(np.count_nonzero(diff)) / len(diff))


def plot_decision_boundary(trues, falses):
    """
    Code to plot the decision boundary

    Decision boundary
    Another helpful technique is to plot the
    decision boundary on top of our predictions
    to see how our labels compare to the actual
    labels. This involves plotting our predicted
    probabilities and coloring them with their true labels.
    """
    fig = plt.figure()
    ax = fig.add_subplot(111)

    # no_of_preds = len(trues) + len(falses)

    ax.scatter([i for i in range(len(trues))], trues, s=25, c='b', marker="o", label='Trues')
    ax.scatter([i for i in range(len(falses))], falses, s=25, c='r', marker="s", label='Falses')

    plt.legend(loc='upper right');
    ax.set_title("Decision Boundary")
    ax.set_xlabel('N/2')
    ax.set_ylabel('Predicted Probability')
    plt.axhline(.5, color='black')
    plt.show()
