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

    def test_loss_sklearn_eg_1(self):
        """
        example taken from sklearn
        https://scikit-learn.org/stable/modules/generated/sklearn.metrics.log_loss.html
        """
        y_true = np.array([1, 0, 0, 1])
        y_pred = np.array([0.9, 0.1, 0.2, 0.65])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.21616187468057912
        )

    def test_loss_sklearn_eg_2(self):
        """
        example taken from sklearn
        https://scikit-learn.org/stable/modules/model_evaluation.html#log-loss

        The first [.9, .1] in y_pred denotes
        90% probability that sample has label 0
        10% probability  that sample has label 1
        """
        y_true = np.array([0, 0, 1, 1])
        y_pred = np.array([.1, .2, .7, .99])
        self.assertAlmostEqual(
            math_ops.log_loss(
                y_true, y_pred
            ),
            0.173807336
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

    def test_loss_stackoverflow_eg1(self):
        """
        test case taken from stackoverflow answer:
        https://stackoverflow.com/questions/58511404/how-to-compute-binary-log-loss-per-sample-of-scikit-learn-ml-model

        confirm that the computation above agrees with
        the total (average) loss as returned by scikit-learn
        """
        predictions = np.array([0.25,0.65,0.2,0.51,
                        0.01,0.1,0.34,0.97])
        targets = np.array([1,0,0,0,
                        0,0,0,1])
        ll = [math_ops.log_loss(np.asarray([x]), np.asarray([y]))
              for (x,y) in zip(targets, predictions)]

        self.assertEqual(
            ll,
            [1.3862943611198906,
            1.0498221244986778,
            0.2231435513142097,
            0.7133498878774648,
            0.01005033585350145,
            0.10536051565782628,
            0.41551544396166595,
            0.030459207484708574]
        )

        # sklearn's log_loss
        ll_sk = 0.4917494284709932

        self.assertEqual(
            ll_sk,
            np.mean(ll)
        )

        ll_falcon = math_ops.log_loss(targets, predictions)

        self.assertEqual(
            ll_sk,
            ll_falcon
        )


if __name__ == '__main__':
    unittest.main()
