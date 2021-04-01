"""
Pytest expects tests to be located in files whose names begin with test_ or end with _test.py
"""

import unittest
import numpy as np
import math_ops


class TestSigmoid(unittest.TestCase):

    def test_sigmoid_large_logit(self):
        logit = 10.0
        est_prob = math_ops.sigmoid(logit)
        self.assertAlmostEqual(
            est_prob,
            0.9999546021312976
        )
        logit = -10.0
        est_prob = math_ops.sigmoid(logit)
        self.assertAlmostEqual(
            est_prob,
            0.0000453978
        )

    def test_sigmoid_small_logit(self):
        logit = 1
        est_prob = math_ops.sigmoid(logit)
        self.assertAlmostEqual(
            est_prob,
            0.7310585786300049
        )
        logit = 0
        est_prob = math_ops.sigmoid(logit)
        self.assertAlmostEqual(
            est_prob,
            0.5
        )


class TestLogLoss(unittest.TestCase):

    def test_loss_simple(self):
        y_true = np.array([1, 1, 0])
        y_pred = np.array([.9, .5, .4])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.43644443999458743
        )

    def test_loss_medium(self):
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([0.9, 0.1, 0.2, 0.65])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.21616187468057912
        )

    def test_ln_zero(self):
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([1, 0, 0, 1])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.0
        )

    def test_ln_almost_zero(self):
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([0.999, 0.0001, 0.0001, 0.99887])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.0005827873164059617
        )

    def test_loss_slp3_5(self):
        """
        test cases from Speech and Language Processing
        by Dan Jurafsky
        https://web.stanford.edu/~jurafsky/slp3/5.pdf
        """
        y_true = np.array([1])
        y_pred = np.array([0.69])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.37106368139083207
        )
        y_true = np.array([0])
        y_pred = np.array([0.69])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            1.1711829815029449
        )


if __name__ == '__main__':
    unittest.main()
