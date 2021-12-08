import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn import preprocessing, svm
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

dataset = pd.read_csv('/opt/falcon/data/dataset/energy_prediction_data/energy.data.norm')
print(dataset.shape)

X = dataset.iloc[:, :-1].values
y = dataset.iloc[:, -1].values

print(X[0, :])
print(y[0])

from sklearn.model_selection import train_test_split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=0)

from sklearn.linear_model import LinearRegression
from sklearn.linear_model import SGDRegressor
# regressor = LinearRegression()
regressor = SGDRegressor(
    loss='squared_error',  #‘squared_error’ loss gives linear regression
    penalty='l2',
    shuffle=False,
    verbose=1,
    max_iter=200,
    learning_rate='constant',
    eta0=0.1,  # initial learning rate for the ‘constant’
    alpha=0.001,
    tol=None,  # do not early stop
)
regressor.fit(X_train, y_train)

print(regressor.intercept_)
print(regressor.coef_)

y_pred = regressor.predict(X_test)

print("y_test = ", y_test)
print("y_pred = ", y_pred)

from sklearn import metrics
print('Mean Absolute Error:', metrics.mean_absolute_error(y_test, y_pred))
print('Mean Squared Error:', metrics.mean_squared_error(y_test, y_pred))
print('Root Mean Squared Error:', np.sqrt(metrics.mean_squared_error(y_test, y_pred)))