import numpy as np

import logreg


"""
compare with the implementation and dataset of beckernick
https://beckernick.github.io/logistic-regression-from-scratch/
"""

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

# features X
X_train = simulated_separableish_features
# labels y
y_train = simulated_labels

print("X_train.shape, y_train.shape = ", X_train.shape, y_train.shape)

# try with full batch
batch_size = len(y_train)

logreg.mini_batch_train(
    X_train,
    y_train,
    batch_size=batch_size,
    lr=5e-5,
    iters=5,
    print_every=1,
)
