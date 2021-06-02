import argparse
import numpy as np
from sklearn.datasets import make_regression, make_classification

'''
Regression doc:     https://scikit-learn.org/stable/modules/generated/sklearn.datasets.make_regression.html
Classification doc: https://scikit-learn.org/stable/modules/generated/sklearn.datasets.make_classification.html
'''

def get_args():
    parser = argparse.ArgumentParser(description='generate datasets for classification/regression')
    parser.add_argument('--is_classification', action='store_true', default=False,
                        help='whether data for classification (default: False, i.e. regression)')
    parser.add_argument('--num_samples', type=int, default=100, help='number of samples generated')
    parser.add_argument('--num_features', type=int, default=10, help='number of features generated')
    parser.add_argument('--num_informative_feats', type=int, default=5, help='number of informative features')
    parser.add_argument('--num_targets', type=int, default=2, help='number of classes for classification (y)')
    parser.add_argument('--save_dir', default='data/', type=str, help='directory to save generated data')

    # Regression hyper-parameters
    parser.add_argument('--bias', default=0., type=float, help='bias term in the underlying linear model.')
    parser.add_argument('--noise', default=1., type=float, 
                        help='standard deviation of the gaussian noise applied to the output')
    parser.add_argument('--effective_rank', type=int, default=None,
                        help='approximate number of singular vectors required to explain most of input data')
    parser.add_argument('--tail_strength', default=2.5, type=float,
                        help='relative importance of fat noisy tail of singular values profile')
    parser.add_argument('--coef', action='store_true', default=False,
                        help='if True, the coefficients of the underlying linear model are returned')

    # Classification hyper-parameters
    parser.add_argument('--num_redundant', type=int, default=2, help='number of redundant features')
    parser.add_argument('--num_clusters_per_class', type=int, default=2, help='number of clusters per class')
    parser.add_argument('--random_label_perc', default=0.01, type=float,
                        help='fraction of samples whose class is assigned randomly')
    parser.add_argument('--class_sep', default=1., type=float, help=' factor multiplying the hypercube size')
    parser.add_argument('--no-hypercube', dest='hypercube', action='store_false',
                        help='if True, the clusters are put on the vertices of a hypercube')
    parser.set_defaults(hypercube=True)

    return parser.parse_args()

args = get_args()
args.save_path = args.save_dir+f'{"cls" if args.is_classification else "reg"}_{args.num_samples}_{args.num_features}_' \
            f'{args.num_informative_feats}_{args.num_targets}.csv'
args.random_seed = 2020

if not args.is_classification:
    # regression data generation
    # X: N*M; y: N; if args.coef==True: args.coef: N
    X, y, coef_ = make_regression(n_samples=args.num_samples, n_features=args.num_features,
                    n_informative=args.num_informative_feats, n_targets=1, bias=args.bias,
                    effective_rank=args.effective_rank, tail_strength=args.tail_strength,
                    noise=args.noise, coef=True, random_state=args.random_seed)

    # reshape y to [N, 1] and pack [X, y] into data
    y.resize(args.num_samples, 1)
    data_ = np.concatenate([X, y], axis=-1)

    # concatenate coef to the first line
    coef = np.concatenate([coef_, [0.]], axis=0).reshape(1, args.num_features+1)
    data = np.concatenate([coef, data_], axis=0)
    if not args.coef: data = data[1:]

else:
    # classification data generation
    # X: N*M; y: N
    assert args.num_targets >= 2
    X, y  = make_classification(n_samples=args.num_samples, n_features=args.num_features, n_informative=args.num_informative_feats,
                n_redundant=args.num_redundant, n_classes=args.num_targets, n_clusters_per_class=args.num_clusters_per_class,
                flip_y=args.random_label_perc, class_sep=args.class_sep, hypercube=args.hypercube, random_state=args.random_seed)

    # reshape y to [N, 1] and pack [X, y] into data
    y.resize(args.num_samples, 1)
    data = np.concatenate([X, y], axis=-1)

# save data to csv
np.savetxt(args.save_path, data, fmt='%.6f', delimiter=',')
