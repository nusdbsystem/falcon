import numpy as np
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score
# from sklearn.metrics import plot_confusion_matrix
# import matplotlib.pyplot as plt

from logreg_from_scratch import *

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_perborgen_toyds import etl_perborgen_toyds


X_train, X_test, y_train, y_test = etl_perborgen_toyds()

# test ml-cheatsheet implemented log reg
"""
NOTE: mlcheatsheet implementation does not consider
bias or intercept term
"""
labels = y_train
features = X_train

# initialize the weights
n_samples, n_features = features.shape
init_weights = np.zeros(n_features)

print("init_weights = ", init_weights)

# hyper-params
learning_rate = 0.1
n_iters = 100
print_every = 1

print("lr = ", learning_rate)
print("n_iters = ", n_iters)

weights, cost_history = train(
    features,
    labels,
    init_weights,
    lr=learning_rate,
    iters=n_iters,
    print_every=print_every,
)

print("trained weights = ", weights)

probabilities = predict(X_test, weights).flatten()
print("probabilities.shape = ", probabilities.shape)
print("probabilities = ", probabilities)

# get actual predicted class
y_pred = classify(probabilities)

print("LogReg regular accuracy:", accuracy_score(y_test, y_pred))

# plot the decision boundary
# plot_decision_boundary(y_test, y_pred)

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
# target_names =  ['malignant' 'benign']
target_names = ['Malignant', 'Benign']
print(classification_report(y_test, y_pred, target_names=target_names))

"""lr 0.1, iter 100
# same as falcon fit_bias=False
iter: 99 cost: 0.4283619008315117
trained weights =  [1.25399924 0.99435344]
cm =
 [[ 9  1]
 [ 6 17]]
"""
