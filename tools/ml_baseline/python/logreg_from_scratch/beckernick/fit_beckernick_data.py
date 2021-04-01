"""
blog link: https://beckernick.github.io/logistic-regression-from-scratch/
"""

import numpy as np
# import matplotlib.pyplot as plt
# %matplotlib inline

from logreg_from_scratch import logistic_regression


np.random.seed(12)
num_observations = 5000

x1 = np.random.multivariate_normal([0, 0], [[1, .75],[.75, 1]], num_observations)
x2 = np.random.multivariate_normal([1, 4], [[1, .75],[.75, 1]], num_observations)

simulated_separableish_features = np.vstack((x1, x2)).astype(np.float32)
simulated_labels = np.hstack((np.zeros(num_observations),
                              np.ones(num_observations)))

print("simulated_separableish_features.shape = ",
      simulated_separableish_features.shape)
print("simulated_labels.shape = ", simulated_labels.shape)
# plt.figure(figsize=(12,8))
# plt.scatter(simulated_separableish_features[:, 0], simulated_separableish_features[:, 1],
#             c = simulated_labels, alpha = .4)

# plt.show()

weights = logistic_regression(simulated_separableish_features, simulated_labels,
                     num_steps = 5, learning_rate = 5e-5, add_intercept=True)

"""beckernick output:
simulated_separableish_features.shape =  (10000, 2)
simulated_labels.shape =  (10000,)

step  0
original weights =  [0. 0. 0.]
scores =  [0. 0. 0. ... 0. 0. 0.]
predictions =  [0.5 0.5 0.5 ... 0.5 0.5 0.5]
gradient =  [0.         0.24658084 1.00051367]
-0.6930940923575263

step  1
original weights =  [0.00000000e+00 1.23290421e-05 5.00256837e-05]
scores =  [-3.66699760e-05 -3.68079121e-05 -6.43833642e-05 ...  2.37846206e-04
  2.14625021e-04  2.62585346e-04]
predictions =  [0.49999083 0.4999908  0.4999839  ... 0.50005946 0.50005366 0.50006565]
gradient =  [-2.67143837e-05  2.46541689e-01  1.00039204e+00]
-0.6930410172881344

step  2
original weights =  [-1.33571918e-09  2.46561265e-05  1.00045286e-04]
scores =  [-7.33367374e-05 -7.36128571e-05 -1.28760163e-04 ...  4.75661094e-04
  4.29222502e-04  5.25136646e-04]
predictions =  [0.49998167 0.4999816  0.49996781 ... 0.50011892 0.50010731 0.50013128]
gradient =  [-5.34251265e-05  2.46502542e-01  1.00027043e+00]
-0.6929879553483616

step  3
original weights =  [-4.00697551e-09  3.69812536e-05  1.50058807e-04]
scores =  [-0.00011    -0.00011041 -0.00019313 ...  0.00071344  0.00064379
  0.00078765]
predictions =  [0.4999725  0.4999724  0.49995172 ... 0.50017836 0.50016095 0.50019691]
gradient =  [-8.01322284e-05  2.46463399e-01  1.00014883e+00]
-0.6929349065348007

step  4
original weights =  [-8.01358693e-09  4.93044236e-05  2.00066249e-04]
scores =  [-0.00014666 -0.00014721 -0.00025749 ...  0.0009512   0.00085833
  0.00105014]
predictions =  [0.49996333 0.4999632  0.49993563 ... 0.5002378  0.50021458 0.50026253]
gradient =  [-1.06835689e-04  2.46424262e-01  1.00002725e+00]
-0.6928818708440444
"""
