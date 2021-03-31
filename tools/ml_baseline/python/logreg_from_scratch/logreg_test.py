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
        X = np.array([3,2,1,3,0,4.19])
        bias = 0.1
        est_prob = logreg._estimate_prob(X, weights, bias)
        self.assertAlmostEqual(
            est_prob,
            0.6969888901292717
        )
