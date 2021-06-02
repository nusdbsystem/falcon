import argparse
from random import randint
from utils import load_from_csv
from utils import AverageMeter, eval_criterion

from sklearn.tree import DecisionTreeClassifier, DecisionTreeRegressor
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.ensemble import RandomForestRegressor, GradientBoostingRegressor

def get_args():
    parser = argparse.ArgumentParser(description='Train with DecisionTree, RandomForest and GradientBoosting')
    parser.add_argument('--num_exps', type=int, default=10, help='number of experiment repeats, report mean value')
    parser.add_argument('--decision_tree', action='store_true', default=False,
                        help='whether to train with DecisionTree (default: False)')
    parser.add_argument('--random_forest', action='store_true', default=False,
                        help='whether to train with RandomForest (default: False)')
    parser.add_argument('--gbdt', action='store_true', default=False,
                        help='whether to train with GDBT (default: False)')

    parser.add_argument('--data_path', default='/home/wuyuncheng/Documents/projects/CollaborativeML/data/air_quality_data/air_quality.data', type=str, help='path to dataset')
    parser.add_argument('--is_classification', action='store_true', default=False,
                        help='whether train as a classification task (default: False)')
    parser.add_argument('--eval_metric', default='l2', type=str, help='evaluation metric for regression tasks')

    # DT/RF/GDBT shared hyper-parameters
    parser.add_argument('--max_depth', type=int, default=6, help='maximum depth of the tree')
    parser.add_argument('--min_samples_split', type=int, default=2,
                        help='minimum number of samples required to split an internal node')
    parser.add_argument('--min_samples_leaf', type=int, default=5,
                        help='minimum number of samples required to be at a leaf node')
    parser.add_argument('--min_impurity_decrease', default=1e-5, type=float,
                        help='node split if induces a decrease of the impurity greater than or equal to this value')

    # DT hyper-parameters
    parser.add_argument('--criterion', default='gini', type=str,
                        help='function to measure quality of split, e.g. gini for classification, mse for regression')

    # RF/GDBT shared hyper-parameters
    parser.add_argument('--n_estimators', type=int, default=16, help='number of trees in the model')

    # GDBT hyper-parameters
    parser.add_argument('--learning_rate', default=1.0, type=float,
                        help='learning rate shrinks the contribution of each tree by learning_rate')
    parser.add_argument('--subsample', default=1.0, type=float,
                        help='fraction of samples to be used for fitting the individual base learners')
    parser.add_argument('--validation_fraction', default=0.0, type=float,
                        help='proportion of training data to set aside as validation set for early stopping')
    parser.add_argument('--loss', default='deviance', type=str,
                        help='loss function to be optimized. ls refers to least squares regression')
    parser.add_argument('--verbose', action='store_false', default=True,
                        help='whether to train with DecisionTree (default: True)')

    return parser.parse_args()

args = get_args()

# loading dataset
X_train, y_train, X_test, y_test = load_from_csv(args.data_path)
print(f'Data shape: \ntrain\t x {X_train.shape},\ty {y_train.shape}\n'
      f'test\t x {X_test.shape},\ty {y_test.shape}\n')

def reload_dateset():
    global X_train, y_train, X_test, y_test
    X_train, y_train, X_test, y_test = load_from_csv(args.data_path)

def train_eval(clf):
    reload_dateset()
    clf.fit(X_train, y_train)

    if args.is_classification:
        train_perf = clf.score(X_train, y_train)
        test_perf = clf.score(X_test, y_test)
    else:
        train_perf = eval_criterion(y_train, clf.predict(X_train), metric=args.eval_metric)
        test_perf = eval_criterion(y_test, clf.predict(X_test), metric=args.eval_metric)

    return train_perf, test_perf

# Decision Tree
if args.decision_tree:
    assert args.criterion in ['gini', 'entropy'] if args.is_classification else \
        args.criterion in ['mse', 'friedman_mse', 'mae']

    train_avg, test_avg = AverageMeter(), AverageMeter()
    for _ in range(args.num_exps):
        if args.is_classification:
            clf = DecisionTreeClassifier(criterion=args.criterion, max_depth=args.max_depth,
                     min_samples_split=args.min_samples_split, min_samples_leaf=args.min_samples_leaf,
                     min_impurity_decrease=args.min_impurity_decrease, random_state=randint(0, 1000000))
        else:
            clf = DecisionTreeRegressor(criterion=args.criterion, max_depth=args.max_depth,
                    min_samples_split=args.min_samples_split, min_samples_leaf=args.min_samples_leaf,
                    min_impurity_decrease=args.min_impurity_decrease, random_state=randint(0, 1000000))
        train_perf, test_perf = train_eval(clf)
        print(train_perf, test_perf)

        train_avg.update(train_perf)
        test_avg.update(test_perf)

    print(f'Decision Tree\t {args.num_exps}-Avg train_perf:\t{train_avg.avg:4f}, test_perf:\t{test_avg.avg:4f}')

# Random Forest
if args.random_forest:
    assert args.criterion in ['gini', 'entropy'] if args.is_classification else \
        args.criterion in ['mse', 'mae']

    train_avg, test_avg = AverageMeter(), AverageMeter()
    for _ in range(args.num_exps):
        if args.is_classification:
            clf = RandomForestClassifier(n_estimators=args.n_estimators, criterion=args.criterion,
                    max_depth=args.max_depth, min_samples_split=args.min_samples_split,
                    min_samples_leaf=args.min_samples_leaf, min_impurity_decrease=args.min_impurity_decrease,
                    random_state=randint(0, 1000000))
        else:
            clf = RandomForestRegressor(n_estimators=args.n_estimators, criterion=args.criterion,
                    max_depth=args.max_depth, min_samples_split=args.min_samples_split,
                    min_samples_leaf=args.min_samples_leaf, min_impurity_decrease=args.min_impurity_decrease,
                    random_state=randint(0, 1000000))
        train_perf, test_perf = train_eval(clf)
        print(train_perf, test_perf)

        train_avg.update(train_perf)
        test_avg.update(test_perf)

    print(f'Random Forest\t {args.num_exps}-Avg train_perf:\t{train_avg.avg:4f}, test_perf:\t{test_avg.avg:4f}')

# GDBT
if args.gbdt:
    assert args.criterion in ['friedman_mse', 'mse']
    assert args.loss in ['deviance', 'exponential'] if args.is_classification else \
        args.loss in ['ls', 'lad', 'huber', 'quantile']

    train_avg, test_avg = AverageMeter(), AverageMeter()
    for _ in range(args.num_exps):
        if args.is_classification:
            clf = GradientBoostingClassifier(loss=args.loss, learning_rate=args.learning_rate,
                     n_estimators=args.n_estimators, subsample=args.subsample,
                     criterion=args.criterion, min_samples_split=args.min_samples_split, max_depth=args.max_depth,
                     min_samples_leaf=args.min_samples_leaf, min_impurity_decrease=args.min_impurity_decrease,
                     validation_fraction=args.validation_fraction, verbose=True, random_state=randint(0, 1000000))
        else:
            clf = GradientBoostingRegressor(loss=args.loss, learning_rate=args.learning_rate,
                    n_estimators=args.n_estimators, subsample=args.subsample, criterion=args.criterion,
                    min_samples_split=args.min_samples_split, max_depth=args.max_depth,
                    min_samples_leaf=args.min_samples_leaf, min_impurity_decrease=args.min_impurity_decrease,
                    validation_fraction=args.validation_fraction, verbose=True, random_state=randint(0, 1000000))
        train_perf, test_perf = train_eval(clf)
        print(train_perf, test_perf)

        train_avg.update(train_perf)
        test_avg.update(test_perf)

    print(f'GDBT\t {args.num_exps}-Avg train_perf:\t{train_avg.avg:4f}, test_perf:\t{test_avg.avg:4f}')
