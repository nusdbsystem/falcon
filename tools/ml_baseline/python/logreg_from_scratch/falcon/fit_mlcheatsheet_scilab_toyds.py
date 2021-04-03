import numpy as np
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn import preprocessing

from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score

import logreg


# load the toy dataset from scilab
with open('/opt/falcon/data/dataset/data_classification.csv') as csv_file:
    ds = np.loadtxt(csv_file, delimiter=",")

"""
This dataset represents 100 samples classified
in two classes as 0 or 1
"""
print("ds shape, dtype = ",
      ds.shape,
      ds.dtype)

# extract the first 2 columns as features X
X = ds[:, :-1]
print("X.shape, X.dtype = ", X.shape, X.dtype)
print("X[0] = ", X[0])

# extract the last column as label y
y = ds[:, -1:].astype(np.int)
# a 1d array was expected. Please change the shape of y to (n_samples, )
y = y.ravel()
print("y.shape, y[:5], y.dtype = ", y.shape, y[:5], y.dtype)
print("=== X and y extraction done ===")

# split the train and test set
# X_train, X_test, y_train, y_test = train_test_split(
#     X,
#     y,
#     test_size=0.2,
#     random_state=42,
# )
X_train, X_test, y_train, y_test = X, X, y, y

print("X_train.shape, X_test.shape = ", X_train.shape, X_test.shape)
print("y_train.shape, y_test.shape = ", y_train.shape, y_test.shape)

print("y_train # 1 () = ", np.count_nonzero(y_train==1))
print("y_train # 0 () = ", np.count_nonzero(y_train==0))
print("y_test # 1 () = ", np.count_nonzero(y_test==1))
print("y_test # 0 () = ", np.count_nonzero(y_test==0))

# test falcon custom implemented log reg
learning_rate = 0.1
n_iters = 3000
# NOTE: scilab adds the intercept column to the feature matrix
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

"""
plot the coss history to compare with
scilab and ml-cheatsheet tutorial
https://ml-cheatsheet.readthedocs.io/en/latest/logistic_regression.html#model-evaluation
https://www.scilab.org/tutorials/machine-learning-%E2%80%93-logistic-regression-tutorial
"""
import matplotlib.pyplot as plt
plt.figure()
plt.plot(
    np.arange(len(cost_history)),
    cost_history,
    label='Logistic Regression (lr = %0.2f)' % learning_rate)
plt.ylabel('Logistic Loss')
plt.xlabel('Iterations')
plt.title('Cost change over iterations')
plt.legend(loc="upper right")
# plt.savefig('Logreg_scilab_tutorial.png')
plt.show()
