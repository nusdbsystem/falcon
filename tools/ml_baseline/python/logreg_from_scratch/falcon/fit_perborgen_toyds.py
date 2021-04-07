from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score

import logreg

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_perborgen_toyds import etl_perborgen_toyds


X_train, X_test, y_train, y_test = etl_perborgen_toyds()

# test falcon custom implemented log reg
learning_rate = 0.1
n_iters = 100
# NOTE: when not using fit_bias, falcon gives
# the same outputs as perborgen implementation
fit_bias = False
print_every = 1

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
print("trained_bias = ", trained_bias)

# predicted probs
y_pred_probs = logreg.predict_proba(X_test, trained_weights, trained_bias, fit_bias=fit_bias)
# print("y_pred_probs = ", y_pred_probs)

# get actual predicted class
y_pred = logreg.predict(X_test, trained_weights, trained_bias, fit_bias=fit_bias)

print("LogReg regular accuracy:", accuracy_score(y_test, y_pred))

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
target_names = ['0', '1']
print(classification_report(y_test, y_pred, target_names=target_names))

"""lr 0.1, iter=100, fit_bias False
iter-99 cost = 0.4283619008315117

trained_weights =  [1.25399924 0.99435344]
trained_bias =  0.0
LogReg regular accuracy: 0.7878787878787878
cm =
 [[ 9  1]
 [ 6 17]]
"""
