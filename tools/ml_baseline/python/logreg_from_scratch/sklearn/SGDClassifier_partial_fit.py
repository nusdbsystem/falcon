"""
Notes
---
using Out-of-core (or “external memory”) learning
with an incremental algorithm (mini-batch GD)

all estimators implementing the partial_fit API in sklearn
can learn incrementally

Key to out-of-core learning is to learn incrementally
from a mini-batch of instances (sometimes called “online learning”)

sklearn.linear_model.SGDClassifier is an online classifier
i.e., one that supports the partial_fit method,
that will be fed with batches of examples

Mini-batch size for SGD based algorithms are truly online
and are not affected by batch size

References
---
https://scikit-learn.org/0.15/modules/scaling_strategies.html
"""
# sklearn
from sklearn.linear_model import SGDClassifier
from sklearn.metrics import confusion_matrix, plot_confusion_matrix, classification_report
# numpy matplotlib
import numpy as np
import matplotlib.pyplot as plt

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_sklearn_breastcancer import etl_sklearn_bc


# load the uci bc dataset from the etl method in utils
X_train, X_test, y_train, y_test = etl_sklearn_bc(
    normalize=False,
    normalization_scheme="MinMaxScaler",
    # normalization_scheme="StandardScaler",
    use_falcon_bc=True,
)

# SGD Logistic Regression from sklearn
# notes on tol:
#   when Gradient Descent has (almost) reached the minimum
#   interrupt the algorithm when the gradient vector becomes tiny
#   smaller than tolerance tol
#   terminate when loss drops by less than 0.001 during one epoch --> tol=1e-3
# notes on warm_start:
#   warm_start=True , when the fit() method is called it continues training
#   where it left off, instead of restarting from scratch
clf = SGDClassifier(
    loss='log',  #‘log’ loss gives logistic regression
    penalty=None,
    shuffle=False,
    verbose=1,
    max_iter=100,
    learning_rate='constant',
    eta0=0.1,  # initial learning rate for the ‘constant’
    tol=None,  # do not early stop
)

# clf.fit(X_train, y_train)

# Main loop : iterate on mini-batchs of examples
n_samples, n_features = X_train.shape

batch_size = 8
iters = 100

all_classes = np.array([0, 1])

cost_history = []

# sanity check with other implementations
# use full batch GD, otherwise default should be
# mini-batch GD
if batch_size < n_samples:
    full_batch_train = False
else:
    print("^^^using Full Batch GD\n")
    full_batch_train = True

# iteratively gradient descent
for iteration in range(iters):
    if full_batch_train:
        X_mini_batch = X_train
        y_mini_batch = y_train
    else:
        # NOTE: mini-batches sampled with replacement
        permutation = np.random.permutation(n_samples)
        X_mini_batch = X_train[permutation][:batch_size]
        y_mini_batch = y_train[permutation][:batch_size]

    # update estimator with examples in the current mini-batch
    clf.partial_fit(X_mini_batch, y_mini_batch, classes=all_classes)


# After being fitted
# The coef_ attribute holds the model parameters:
print("clf.coef_ = ", clf.coef_)
# The intercept_ attribute holds the intercept (aka offset or bias):
print("clf.intercept_ = ", clf.intercept_)

"""
NOTE: the SGDClassifier .fit() method every
run gives different coef_ and intercept_,
meaning the model is trained to have slightly
different model parameters, even though when
the X_train, y_train are fixed!

cdef class Log(Classification):
    Logistic regression loss for binary classification with y in {-1, 1}

NOTE: SGDClassifer is updating the weights with every single
sample gradient descent. the initial weight is all [0,0...,0],
but after each i in n_samples, sumloss += loss.loss(p, y)

https://stackoverflow.com/questions/55552139/sklearn-implementation-for-batch-gradient-descend
"""

# show the classes
print("clf.classes_ = ", clf.classes_)

# The first column is the probability of the predicted output being zero, that is 1 - 𝑝(𝑥)
# The second column is the probability that the output is one, or 𝑝(𝑥).
print("x_test's first 3 predicted probabilities = \n", clf.predict_proba(X_test)[:3])

# get actual predicted class
y_pred = clf.predict(X_test)

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
plot_confusion_matrix(
    clf, X_test, y_test,
    display_labels=['Malignant', 'Benign'],
    cmap=plt.cm.Blues,
)

print(classification_report(y_test, y_pred))
