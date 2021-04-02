"""
compare with the implementation and dataset of beckernick
https://beckernick.github.io/logistic-regression-from-scratch/
"""
import numpy as np
from logreg_from_scratch import LogisticRegression

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_beckernick_dataset import etL_beckernick_ds


X_train, y_train = etL_beckernick_ds()

regressor = LogisticRegression(learning_rate=5e-5, n_iters=5)
regressor.fit(X_train, y_train)

"""python-engineer output:
simulated_separableish_features.shape =  (10000, 2)
simulated_labels.shape =  (10000,)
X_train.shape, y_train.shape =  (10000, 2) (10000,)
orginial weights =  [0. 0.]
original bias =  0
dw =  [-0.24658084 -1.00051367]
db =  0.0
orginial weights =  [1.23290421e-05 5.00256837e-05]
original bias =  0.0
dw =  [-0.24654169 -1.00039204]
db =  2.6714383660851128e-05
orginial weights =  [2.46561265e-05 1.00045286e-04]
original bias =  -1.3357191830425565e-09
dw =  [-0.24650254 -1.00027043]
db =  5.342512654556231e-05
orginial weights =  [3.69812536e-05 1.50058807e-04]
original bias =  -4.0069755103206725e-09
dw =  [-0.2464634  -1.00014883]
db =  8.01322283821719e-05
orginial weights =  [4.93044236e-05 2.00066249e-04]
original bias =  -8.013586929429267e-09
dw =  [-0.24642426 -1.00002725]
db =  0.00010683568889941171
"""
