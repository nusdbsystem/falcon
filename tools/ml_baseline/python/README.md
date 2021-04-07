# Python Implementations of ML Baseline

## Falcon Custom Logistic Regression Implementation

Call the Python scripts under `tools/ml_baseline/python/logreg_from_scratch/falcon/fit_*.py` to see mini-batch gradient descent training in action.

The mini-batch GD implemented is mini-batch sampling with replacement. The option `fit_bias` toggles whether to include a _bias_ or _intercept_ constant term in training.

References:
- [ML cheatsheet Logistic Regression](https://ml-cheatsheet.readthedocs.io/en/latest/logistic_regression.html)
- [Logistic Regression implementation by python-engineer github/youtube](https://github.com/python-engineer/MLfromscratch/blob/master/mlfromscratch/logistic_regression.py)
- [Logistic Regression implementation by beckernick](https://beckernick.github.io/logistic-regression-from-scratch/)
- [Logistic Regression implementation by perborgen](https://github.com/perborgen/LogisticRegression)
- [Logistic Regression with Mini-batch GD by iamkucuk](https://github.com/iamkucuk/Logistic-Regression-With-Mini-Batch-Gradient-Descent/blob/master/logistic_regression_notebook.ipynb)

## Unit tests for Falcon's Custom Implementation

to run unit-tests:

```sh
python3 -m unittest discover -s ./logreg_from_scratch/falcon/ -p "*_test.py"
```


or use `pytest`

```sh
# install pytest
pip3 install -U pytest
# check pytest version
pytest --version

# invoke by just
pytest
```
