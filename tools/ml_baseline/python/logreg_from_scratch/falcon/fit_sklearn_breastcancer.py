"""
modified from
https://github.com/python-engineer/MLfromscratch/blob/master/mlfromscratch/logistic_regression_tests.py
"""
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score
# from sklearn.metrics import plot_confusion_matrix
# import matplotlib.pyplot as plt

import logreg

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_sklearn_breastcancer import etl_sklearn_bc


# load the uci bc dataset from the etl method in utils
X_train, X_test, y_train, y_test = etl_sklearn_bc(
    normalize=True,
    normalization_scheme="MinMaxScaler",
)

# test falcon custom implemented log reg
learning_rate = 0.001
n_iters = 1000
fit_bias = True
print_every = 100

# try with full batch
batch_size = len(y_train)

trained_weights, trained_bias, cost_history = logreg.mini_batch_train(
    X_train,
    y_train,
    batch_size=batch_size,
    lr=learning_rate,
    iters=n_iters,
    fit_bias=fit_bias,
    print_every=print_every,
)

print("trained_weights = ", trained_weights)

# predicted probs
y_pred_probs = logreg.predict_proba(X_test, trained_weights, trained_bias, fit_bias=fit_bias)
print("y_pred_probs = ", y_pred_probs)

# get actual predicted class
y_pred = logreg.predict(X_test, trained_weights, trained_bias, fit_bias=fit_bias)

print("LogReg regular accuracy:", accuracy_score(y_test, y_pred))

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
# target_names =  ['malignant' 'benign']
target_names = ['Malignant', 'Benign']
print(classification_report(y_test, y_pred, target_names=target_names))

"""for learning_rate=0.001, n_iters=1000:
original weights =  [ 3.24593714e-01  4.23758334e-01  1.88581868e+00  8.00274678e-01
  2.92008615e-03 -1.48569894e-03 -5.91676360e-03 -2.52697536e-03
  5.49386149e-03  2.35089445e-03  1.28691948e-03  3.09549283e-02
 -8.01440966e-03 -8.13891926e-01  1.54591954e-04 -3.38797128e-04
 -6.64366069e-04 -8.33143778e-05  4.93915050e-04  5.10085798e-05
  3.41355328e-01  5.28743663e-01  1.90487277e+00 -1.08533027e+00
  3.57488426e-03 -6.48608609e-03 -1.27011439e-02 -2.95368700e-03
  7.06695882e-03  2.15338126e-03]
bias =  0.042394919617828083

dw =  [5.89179113e-01 1.05304523e+00 3.95439862e+00 2.68960591e+01
 4.70924056e-03 7.89433463e-03 8.49368417e-03 3.76958254e-03
 8.92281290e-03 2.96176926e-03 1.55522979e-02 7.35237586e-02
 1.28095959e-01 1.36728377e+00 4.37013512e-04 2.26381207e-03
 2.89499017e-03 9.02653155e-04 1.26496044e-03 2.82339836e-04
 6.40716426e-01 1.45318403e+00 4.47641294e+00 3.09316998e+01
 6.81654041e-03 2.22870063e-02 2.55932026e-02 8.29420208e-03
 1.57033222e-02 4.69935241e-03]
db =  0.04087920182310455

new_weights =  [ 3.24004535e-01  4.22705289e-01  1.88186428e+00  7.73378619e-01
  2.91537691e-03 -1.49359327e-03 -5.92525728e-03 -2.53074494e-03
  5.48493868e-03  2.34793268e-03  1.27136718e-03  3.08814045e-02
 -8.14250562e-03 -8.15259209e-01  1.54154940e-04 -3.41060941e-04
 -6.67261059e-04 -8.42170309e-05  4.92650090e-04  5.07262400e-05
  3.40714612e-01  5.27290479e-01  1.90039636e+00 -1.11626197e+00
  3.56806772e-03 -6.50837310e-03 -1.27267371e-02 -2.96198120e-03
  7.05125550e-03  2.14868191e-03]
new_bias =  0.042354040416004976

iter-999 cost = 2.6635648790876165

LogReg regular accuracy: 0.9473684210526315
cm =
 [[43  0]
 [ 6 65]]
              precision    recall  f1-score   support

   Malignant       0.88      1.00      0.93        43
      Benign       1.00      0.92      0.96        71

    accuracy                           0.95       114
   macro avg       0.94      0.96      0.95       114
weighted avg       0.95      0.95      0.95       114
"""

"""with X_train standardization, learning_rate=0.001, n_iters=1000:
iter-999 cost = 0.2548839509475849

LogReg regular accuracy: 0.9736842105263158
cm =
 [[42  1]
 [ 2 69]]
              precision    recall  f1-score   support

   Malignant       0.95      0.98      0.97        43
      Benign       0.99      0.97      0.98        71

    accuracy                           0.97       114
   macro avg       0.97      0.97      0.97       114
weighted avg       0.97      0.97      0.97       114
"""

"""with minmax normalization, learning_rate=0.001, n_iters=1000
iter-0 cost = 0.693115541845784
iter-999 cost = 0.6642076761657234

LogReg regular accuracy: 0.7280701754385965
cm =
 [[12 31]
 [ 0 71]]
"""
