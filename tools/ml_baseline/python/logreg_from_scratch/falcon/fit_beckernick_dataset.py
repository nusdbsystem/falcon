"""
compare with the implementation and dataset of beckernick
https://beckernick.github.io/logistic-regression-from-scratch/
"""
import numpy as np

import logreg

import sys
sys.path.append("..")  # Adds higher directory to python modules path.
from utils.etl_beckernick_dataset import etL_beckernick_ds


X_train, y_train = etL_beckernick_ds()

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
