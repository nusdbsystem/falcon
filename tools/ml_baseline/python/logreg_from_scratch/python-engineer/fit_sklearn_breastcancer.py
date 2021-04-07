"""
modified from
https://github.com/python-engineer/MLfromscratch/blob/master/mlfromscratch/logistic_regression_tests.py
"""
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score
# from sklearn.metrics import plot_confusion_matrix
# import matplotlib.pyplot as plt

from logreg_from_scratch import LogisticRegression

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_sklearn_breastcancer import etl_sklearn_bc


# load the uci bc dataset from the etl method in utils
X_train, X_test, y_train, y_test = etl_sklearn_bc(
    normalize=False,
    normalization_scheme=None,
)

# test python-engineer implemented log reg
clf = LogisticRegression(learning_rate=0.001, n_iters=1000)
clf.fit(X_train, y_train)

# get actual predicted class
y_pred = clf.predict(X_test)

print("LogReg regular accuracy:", accuracy_score(y_test, y_pred))

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
# target_names =  ['malignant' 'benign']
target_names = ['Malignant', 'Benign']
print(classification_report(y_test, y_pred, target_names=target_names))

"""for learning_rate=0.001, n_iters=1000:
orginial weights =  [ 3.24593712e-01  4.23758333e-01  1.88581867e+00  8.00274686e-01
  2.92008614e-03 -1.48569890e-03 -5.91676353e-03 -2.52697533e-03
  5.49386152e-03  2.35089444e-03  1.28691952e-03  3.09549286e-02
 -8.01440896e-03 -8.13891914e-01  1.54591957e-04 -3.38797116e-04
 -6.64366049e-04 -8.33143690e-05  4.93915071e-04  5.10085808e-05
  3.41355326e-01  5.28743662e-01  1.90487277e+00 -1.08533028e+00
  3.57488427e-03 -6.48608591e-03 -1.27011437e-02 -2.95368690e-03
  7.06695901e-03  2.15338128e-03]
original bias =  0.042394919409181185
dw =  [5.89179180e-01 1.05304528e+00 3.95439906e+00 2.68960633e+01
 4.70924088e-03 7.89433510e-03 8.49368475e-03 3.76958291e-03
 8.92281354e-03 2.96176946e-03 1.55522990e-02 7.35237627e-02
 1.28095967e-01 1.36728390e+00 4.37013527e-04 2.26381216e-03
 2.89499028e-03 9.02653197e-04 1.26496049e-03 2.82339849e-04
 6.40716497e-01 1.45318410e+00 4.47641341e+00 3.09317045e+01
 6.81654080e-03 2.22870072e-02 2.55932039e-02 8.29420269e-03
 1.57033231e-02 4.69935269e-03]
db =  0.040879205049607584
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
