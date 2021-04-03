import numpy as np
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score
# from sklearn.metrics import plot_confusion_matrix
# import matplotlib.pyplot as plt

from logreg_from_scratch import *

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_sklearn_breastcancer import etl_sklearn_bc


# load the uci bc dataset from the etl method in utils
X_train, X_test, y_train, y_test = etl_sklearn_bc(
    normalize=True,
    # normalization_scheme="StandardScaler",
    normalization_scheme="MinMaxScaler"
)

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

"""for minmaxscaled, learning_rate=0.001, n_iters=1000:
lr =  0.001
n_iters =  1000
iter: 0 cost: 0.6931320710010533
iter: 100 cost: 0.6916281120389617
iter: 200 cost: 0.6901376404927786
iter: 300 cost: 0.6886601461320804
iter: 400 cost: 0.6871951660213071
iter: 500 cost: 0.6857422795823087
iter: 600 cost: 0.6843011041695194
iter: 700 cost: 0.682871291105704
iter: 800 cost: 0.6814525221312904
iter: 900 cost: 0.6800445062249375
trained weights =  [-0.01800137  0.0130034  -0.01936008 -0.02317533  0.02810183 -0.01304994
 -0.03347547 -0.03883059  0.0202696   0.03591781 -0.01263391  0.02429527
 -0.01165511 -0.01355397  0.02610757  0.00588792  0.00243397  0.00772656
  0.03033567  0.01118609 -0.02804594  0.01073708 -0.02762312 -0.02722346
  0.02103394 -0.01560806 -0.02326386 -0.03326095  0.00812407  0.00906768]
LogReg regular accuracy: 0.8157894736842105
cm =
 [[42  1]
 [20 51]]
"""
