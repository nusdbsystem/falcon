import numpy as np
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn import preprocessing
from sklearn.metrics import confusion_matrix, classification_report
# from sklearn.metrics import plot_confusion_matrix
# import matplotlib.pyplot as plt

from logreg_from_scratch import logistic_regression, sigmoid


def regular_accuracy(y_true, y_pred):
    regular_accuracy = np.sum(y_true == y_pred) / len(y_true)
    return regular_accuracy


# Test with sklearn's breast cancer dataset
bc = datasets.load_breast_cancer()
print("list of breast_cancer keys() =\n", list(bc.keys()))

# Class Distribution: 212 - Malignant, 357 - Benign
print("target_names = ", bc["target_names"])
# target_names =  ['malignant' 'benign']

print("DESCR = ")
print(bc["DESCR"])

X, y = bc.data, bc.target
print("X.shape, X.dtype = ", X.shape, X.dtype)
print("y.shape, y.dtype = ", y.shape, y.dtype)

# Class Distribution: 212 - Malignant, 357 - Benign
# malignant class is 0
np.testing.assert_equal(np.count_nonzero(y==0), 212)
# benign class is 1
np.testing.assert_equal(np.count_nonzero(y==1), 357)


print("feature_names = ", bc["feature_names"])

# split the train and test set
X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42,
)

print("X_train.shape, X_test.shape = ", X_train.shape, X_test.shape)
print("y_train.shape, y_test.shape = ", y_train.shape, y_test.shape)

print("y_train # 1 (benign) = ", np.count_nonzero(y_train==1))
print("y_train # 0 (malignant) = ", np.count_nonzero(y_train==0))
print("y_test # 1 (benign) = ", np.count_nonzero(y_test==1))
print("y_test # 0 (malignant) = ", np.count_nonzero(y_test==0))

"""
X_train.shape, X_test.shape =  (455, 30) (114, 30)
y_train.shape, y_test.shape =  (455,) (114,)
y_train # 1 (benign) =  286
y_train # 0 (malignant) =  169
y_test # 1 (benign) =  71
y_test # 0 (malignant) =  43
"""

print("before normalization, X_train = ", X_train)
print("before normalization, X_test = ", X_test)

# Data Standardization
# NOTE: sklearn's transform's fit() just calculates the parameters
# (e.g. mean and std in case of StandardScaler)
# and saves them as an internal object's state
scaler = preprocessing.StandardScaler().fit(X_train)
print("scaler.mean_ = ", scaler.mean_)
print("scaler.scale_ = ", scaler.scale_)

# NOTE: Afterwards, you can call its transform() method
# to apply the transformation to any particular set of examples
# from https://datascience.stackexchange.com/questions/12321/whats-the-difference-between-fit-and-fit-transform-in-scikit-learn-models
X_train = scaler.transform(X_train)
print("After normalization, X_train = ", X_train)
X_test = scaler.transform(X_test)
print("After normalization, X_test = ", X_test)

# test beckernick implemented log reg
learning_rate = 0.001
n_iters = 1000

weights = logistic_regression(X_train, y_train,
                     num_steps = n_iters, learning_rate = learning_rate, add_intercept=True)

print("trained weights = ", weights)

intercept = np.ones((X_test.shape[0], 1))
features = np.hstack((intercept, X_test))
scores = np.dot(features, weights)
# print("scores = ", scores)
predictions = sigmoid(scores)
# print("predictions = ", predictions)

# get actual predicted class
y_pred = [1 if i > 0.5 else 0 for i in predictions]

print("LogReg regular accuracy:", regular_accuracy(y_test, y_pred))

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
# target_names =  ['malignant' 'benign']
target_names = ['Malignant', 'Benign']
print(classification_report(y_test, y_pred, target_names=target_names))

"""for learning_rate=0.001, n_iters=1000:
-40.15168201218089
/opt/falcon/tools/ml_baseline/python/logreg_from_scratch/beckernick/logreg_from_scratch.py:10: RuntimeWarning: overflow encountered in exp
  return 1 / (1 + np.exp(-scores))
/opt/falcon/tools/ml_baseline/python/logreg_from_scratch/beckernick/logreg_from_scratch.py:17: RuntimeWarning: overflow encountered in exp
  ll = np.sum( target*scores - np.log(1 + np.exp(scores)) )/len(target)
-inf
-9.09623799535008
-9.66917256053947
-8.679821788354722
-5.1600737735510105
-4.79922292444449
-4.442088913164089
-4.202957295484916
-4.799259162882295
trained weights =  [ 4.23540402e-02  3.24004533e-01  4.22705288e-01  1.88186427e+00
  7.73378623e-01  2.91537690e-03 -1.49359324e-03 -5.92525722e-03
 -2.53074491e-03  5.48493870e-03  2.34793267e-03  1.27136722e-03
  3.08814048e-02 -8.14250493e-03 -8.15259198e-01  1.54154944e-04
 -3.41060928e-04 -6.67261039e-04 -8.42170222e-05  4.92650111e-04
  5.07262409e-05  3.40714610e-01  5.27290478e-01  1.90039635e+00
 -1.11626198e+00  3.56806773e-03 -6.50837292e-03 -1.27267369e-02
 -2.96198110e-03  7.05125569e-03  2.14868192e-03]
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
999-th iter cost = -0.2548839509475849
trained weights =  [ 0.09823399 -0.15457617 -0.10283256 -0.15572055 -0.14802736 -0.0696766
 -0.10066808 -0.12839554 -0.15981715 -0.06040731  0.02987752 -0.10980129
  0.00480462 -0.10306172 -0.10353987  0.01784057 -0.01646969 -0.00824143
 -0.05268934  0.01135819  0.02815828 -0.16807408 -0.11916568 -0.16686432
 -0.15581365 -0.09643333 -0.11183074 -0.12577065 -0.16392033 -0.10122089
 -0.05226781]
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
