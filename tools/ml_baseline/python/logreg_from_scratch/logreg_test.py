"""
Pytest expects tests to be located in files whose names begin with test_ or end with _test.py
"""

import unittest
import numpy as np
import logreg


class TestEstimateProb(unittest.TestCase):
    """
    test cases from Speech and Language Processing
    by Dan Jurafsky
    https://web.stanford.edu/~jurafsky/slp3/5.pdf
    """
    def test_estimate_prob(self):
        weights = np.array([2.5,-5.0,-1.2,0.5,2.0,0.7])
        # single instance
        X = np.array([3,2,1,3,0,4.19])
        bias = 0.1
        est_prob = logreg._estimate_prob(X, weights, bias)
        self.assertAlmostEqual(
            est_prob,
            0.6969888901292717
        )

    def test_estimate_prob_array(self):
        weights = np.array([2.5,-5.0,-1.2,0.5,2.0,0.7])
        # multiple instances
        X = np.array([
            [3,2,1,3,0,4.19],
            [3,2,1,3,0,4.19],
        ])
        bias = 0.1
        est_prob = logreg._estimate_prob(X, weights, bias)
        self.assertTrue(
            # Returns True if two arrays are
            # element-wise equal within a tolerance.
            np.allclose(
                est_prob,
                np.array([0.6969888901292717, 0.69698889])
            )
        )


class TestPredict(unittest.TestCase):
    def test_predict(self):
        weights = np.array([2.5,-5.0,-1.2,0.5,2.0,0.7])
        # X needs to be an array of instances
        X = np.array([
            [3,2,1,3,0,4.19],
        ])
        bias = 0.1

        predicted_labels = logreg.predict(X, weights, bias)
        self.assertTrue(
            np.array_equal(
                predicted_labels,
                np.array([1])
            )
        )


class TestPredictProba(unittest.TestCase):
    def test_predict_proba(self):
        weights = np.array([2.5,-5.0,-1.2,0.5,2.0,0.7])
        # X needs to be an array of instances
        X = np.array([
            [3,2,1,3,0,4.19],
        ])
        bias = 0.1

        predicted_labels = logreg.predict_proba(X, weights, bias)
        self.assertTrue(
            np.allclose(
                predicted_labels,
                np.array([0.69698889])
            )
        )