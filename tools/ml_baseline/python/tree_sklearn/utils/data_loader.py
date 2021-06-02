from numpy import genfromtxt
from sklearn.model_selection import train_test_split

def load_from_csv(data_path, test_perc=0.2, delimiter=','):
    '''
    assume label on the last feature dimension
    :param test_perc:   percentage of data used for validation
    :return:
    '''
    data = genfromtxt(data_path, delimiter=delimiter)
    X, y = data[:, :-1], data[:, -1]
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=test_perc)
    return X_train, y_train, X_test, y_test
