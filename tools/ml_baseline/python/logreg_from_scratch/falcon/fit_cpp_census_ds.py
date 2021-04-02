"""
compare with C++ implementations of Logistic Regression.
@ /opt/falcon/tools/ml_baseline/cpp
"""
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score
# from sklearn.metrics import plot_confusion_matrix
# import matplotlib.pyplot as plt

import logreg

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_cpp_census_ds import etl_cpp_census_ds


# load the uci bc dataset from the etl method in utils
X_train, X_test, y_train, y_test = etl_cpp_census_ds()

# test falcon custom implemented log reg
"""
cpp code setting:
    // learning rate
    double learning_rate = 0.01;
    // number of iterations
    int num_iter = 1000;
"""
learning_rate = 0.01
n_iters = 10000
print_every = 100

# try with full batch
batch_size = len(y_train)

trained_weights, trained_bias, cost_history = logreg.mini_batch_train(
    X_train,
    y_train,
    batch_size=batch_size,
    lr=learning_rate,
    iters=n_iters,
    print_every=print_every,
)

# get actual predicted class
y_pred = logreg.predict(X_test, trained_weights, trained_bias)

print("LogReg regular accuracy:", accuracy_score(y_test, y_pred))

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)

target_names = ['0', '1']
print(classification_report(y_test, y_pred, target_names=target_names))

"""for learning_rate=0.01, n_iters=10000:
^^^using Full Batch GD

iter-0 cost = 0.6917887258610168

...

iter-9899 cost = 0.40038435081258505

iter-9999 cost = 0.40034552943260293

LogReg regular accuracy: 0.8239681750372949
cm =
 [[4276  227]
 [ 835  695]]
              precision    recall  f1-score   support

           0       0.84      0.95      0.89      4503
           1       0.75      0.45      0.57      1530

    accuracy                           0.82      6033
   macro avg       0.80      0.70      0.73      6033
weighted avg       0.82      0.82      0.81      6033
"""
