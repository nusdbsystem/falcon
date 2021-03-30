"""
Pytest expects tests to be located in files whose names begin with test_ or end with _test.py
"""

import unittest
import numpy as np
import log_loss


class TestLogLoss(unittest.TestCase):

    def test_loss_simple(self):
        y_true = np.array([1, 1, 0])
        y_pred = np.array([.9, .5, .4])
        self.assertAlmostEqual(
            log_loss.log_loss(
                y_true, y_pred
            ),
            0.43644443999458743
        )

    def test_loss_medium(self):
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([0.9, 0.1, 0.2, 0.65])
        self.assertAlmostEqual(
            log_loss.log_loss(
                y_true, y_pred
            ),
            0.21616187468057912
        )

    def test_ln_zero(self):
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([1, 0, 0, 1])
        self.assertAlmostEqual(
            log_loss.log_loss(
                y_true, y_pred
            ),
            0.0
        )

    def test_ln_almost_zero(self):
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([0.999, 0.0001, 0.0001, 0.99887])
        self.assertAlmostEqual(
            log_loss.log_loss(
                y_true, y_pred
            ),
            0.0005827873164059617
        )


if __name__ == '__main__':
    unittest.main()
