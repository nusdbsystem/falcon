import numpy as np
# import matplotlib.pyplot as plt
# %matplotlib inline


def etL_beckernick_ds():
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
    # plt.figure(figsize=(12,8))
    # plt.scatter(simulated_separableish_features[:, 0], simulated_separableish_features[:, 1],
    #             c = simulated_labels, alpha = .4)

    # plt.show()

    # features X
    X_train = simulated_separableish_features
    # labels y
    y_train = simulated_labels

    print("X_train.shape, y_train.shape = ", X_train.shape, y_train.shape)

    return X_train, y_train
