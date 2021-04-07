from sklearn.metrics import confusion_matrix, classification_report
from sklearn.metrics import accuracy_score

from logreg_from_scratch import Logistic_Regression, Hypothesis

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_perborgen_toyds import etl_perborgen_toyds


X_train, X_test, y_train, y_test = etl_perborgen_toyds()

# These are the initial guesses for theta as well as the learning rate of the algorithm
# A learning rate too low will not close in on the most accurate values within a reasonable number of iterations
# An alpha too high might overshoot the accurate values or cause irratic guesses
# Each iteration increases model accuracy but with diminishing returns,
# and takes a signficicant coefficient times O(n)*|Theta|, n = dataset length
initial_theta = [0,0]
alpha = 0.1
iterations = 100
print_every = 10

trained_theta = Logistic_Regression(X_train, y_train, alpha,
                                    initial_theta, iterations,
                                    print_every=print_every)


# get actual predicted class
y_pred = []
length = len(X_test)
for i in range(length):
    prediction = round(Hypothesis(X_test[i], trained_theta))
    y_pred.append(prediction)

print("LogReg regular accuracy:", accuracy_score(y_test, y_pred))

# show the confusion matrix
cm = confusion_matrix(y_test, y_pred)
print("cm =\n", cm)
target_names = ['0', '1']
print(classification_report(y_test, y_pred, target_names=target_names))

"""lr 0.1, iter 100
final theta =  [1.2539992366859942, 0.9943534363253964]
LogReg regular accuracy: 0.7878787878787878
cm =
 [[ 9  1]
 [ 6 17]]
"""
