from sklearn.neural_network import MLPClassifier
from sklearn.datasets import make_classification
from sklearn.model_selection import train_test_split
from numpy import genfromtxt

data = genfromtxt("/opt/falcon/data/dataset/breast_cancer_data/breast_cancer.data.norm", delimiter=',')
X, y = data[:, :-1], data[:, -1]
X_train, X_test, y_train, y_test = train_test_split(X, y, stratify=y, random_state=1)
clf = MLPClassifier(random_state=1,
                    hidden_layer_sizes=(2),
                    activation="logistic",
                    alpha=0.1,
                    solver="sgd",
                    learning_rate_init=0.1,
                    batch_size=8,
                    max_iter=2).fit(X_train, y_train)
print(clf.n_outputs_)
print(clf.out_activation_)
clf.predict_proba(X_test[:1])
clf.predict(X_test[:5, :])
print(clf.score(X_test, y_test))