"""
modified from
https://github.com/python-engineer/MLfromscratch/blob/master/mlfromscratch/logistic_regression_tests.py
"""
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn import preprocessing


def etl_sklearn_bc(normalize=True,
                   normalization_scheme="MinMaxScaler",
                   use_falcon_bc=False):
    """
    Parameters:
    # Whether to normalize the data
    normalize = True
        # if to normalize, choose the normalization scheme
        # choose from {MinMaxScaler | StandardScaler}
    # whether to use falcon's breast cancer data

    return the preprocessed train test split
    of UCI breastcancer dataset
    """
    if use_falcon_bc:
        """
        load the normalized UCI breast cancer data
        original data from sklearn load_creast_cancer()
        preprocessed with falcon's cpp minmax-scaler
        """
        # NOTE: to test on partial features, use client0's data
        # @ '/opt/falcon/data/dataset/breast_cancer_data/client0/client.txt'
        with open('/opt/falcon/data/dataset/breast_cancer_data/breast_cancer.data.norm') as csv_file:
            bc_data_norm = np.loadtxt(csv_file, delimiter=",")

        print("bc_data_norm shape, dtype = ",
              bc_data_norm.shape,
              bc_data_norm.dtype)

        # extract the first 30 columns as features X
        # NOTE: to test on partial features, can also simply
        # only use partial features bc_data_norm[:, ?:-1]
        X = bc_data_norm[:, :-1]
        print("X.shape, X.dtype = ", X.shape, X.dtype)
        print("X[0] = ", X[0])

        # extract the last column as label y
        y = bc_data_norm[:, -1:].astype(np.int)
        # a 1d array was expected. Please change the shape of y to (n_samples, )
        y = y.ravel()
        print("y.shape, y[:5], y.dtype = ", y.shape, y[:5], y.dtype)
        print("=== X and y extraction done ===")
    else:
        # Test with sklearn's breast cancer dataset
        bc = datasets.load_breast_cancer()
        print("list of breast_cancer keys() =\n", list(bc.keys()))

        # Class Distribution: 212 - Malignant, 357 - Benign
        print("target_names = ", bc["target_names"])
        # target_names =  ['malignant' 'benign']

        print("DESCR = ")
        print(bc["DESCR"])

        print("feature_names = ", bc["feature_names"])

        X, y = bc.data, bc.target
        print("X.shape, X.dtype = ", X.shape, X.dtype)
        print("y.shape, y.dtype = ", y.shape, y.dtype)

    # Class Distribution: 212 - Malignant, 357 - Benign
    # malignant class is 0
    np.testing.assert_equal(np.count_nonzero(y==0), 212)
    # benign class is 1
    np.testing.assert_equal(np.count_nonzero(y==1), 357)

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

    if normalize:
        # Data Standardization
        # NOTE: sklearn's transform's fit() just calculates the parameters
        # (e.g. mean and std in case of StandardScaler)
        # and saves them as an internal object's state

        if normalization_scheme == "StandardScaler":
            scaler = preprocessing.StandardScaler().fit(X_train)
            print("scaler.mean_ = ", scaler.mean_)
            print("scaler.scale_ = ", scaler.scale_)
        elif normalization_scheme == "MinMaxScaler":
            scaler = preprocessing.MinMaxScaler().fit(X_train)
            print("scaler.min_ = ", scaler.min_)
            print("scaler.data_min_", scaler.data_min_)
            print("scaler.data_max_", scaler.data_max_)
            print("scaler.scale_ = ", scaler.scale_)

        # NOTE: Afterwards, you can call its transform() method
        # to apply the transformation to any particular set of examples
        # from https://datascience.stackexchange.com/questions/12321/whats-the-difference-between-fit-and-fit-transform-in-scikit-learn-models
        X_train = scaler.transform(X_train)
        print("After normalization, X_train = ", X_train)
        X_test = scaler.transform(X_test)
        print("After normalization, X_test = ", X_test)

    return X_train, X_test, y_train, y_test
