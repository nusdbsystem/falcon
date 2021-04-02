"""
Pytest expects tests to be located in files whose names begin with test_ or end with _test.py
"""

import unittest
import numpy as np
import logreg
import math_ops


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


class TestDecisionBoundary(unittest.TestCase):
    def test_decision_boundary(self):
        thres = 0.5
        self.assertEqual(
            logreg.decision_boundary(0.6, thres),
            1
        )
        self.assertEqual(
            logreg.decision_boundary(0.5, thres),
            1
        )
        self.assertEqual(
            logreg.decision_boundary(0.08, thres),
            0
        )
        thres = 0.8  # different threshold
        self.assertEqual(
            logreg.decision_boundary(0.6, thres),
            0
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


class TestGradientDescent(unittest.TestCase):
    """
    test cases from Speech and Language Processing
    by Dan Jurafsky
    https://web.stanford.edu/~jurafsky/slp3/5.pdf
    5.4    Gradient Descent
    """
    def test_gradient_descent(self):
        # a single observation x
        X = np.array([
            [3, 2],
        ])
        # correct value is y=1
        y = 1
        # initial weights and bias
        weights = np.zeros(2,)
        bias = 0
        # initial learnig rate = 0.1
        lr = 0.1

        new_weights, new_bias = logreg.gradient_descent(
            X, y, weights, bias, lr
        )

        """
        So  after  one  step  of  gradient  descent, 
        the  weights  have  shifted  to  be:
        w1=.15, w2=.1, b=.05
        """
        np.testing.assert_array_almost_equal(
            new_weights,
            np.array([0.15, 0.1])
        )

        self.assertEqual(
            new_bias, 0.05
        )


class TestTrainingLoss(unittest.TestCase):
    """
    calculate the loss on a training set

    using example from https://web.stanford.edu/~jurafsky/slp3/5.pdf
    5.1.1    Example: sentiment classification

    NOTE: the textbook rounded the prob to 2 dp,
    so the probs are .69 and .31, and loss is not very accurate
    in this test-case, we calculate the exact loss
    """
    def test_training_loss(self):
        # 6 features for this one instance are
        # X shape is (1, 6)
        X_train = np.array([
            [3,2,1,3,0,4.19],
        ])
        # 6 weights corresponding to the 6 features are
        weights = np.array([2.5,-5.0,-1.2,0.5,2.0,0.7])
        bias = 0.1

        fit_bias = True

        # suppose the correct gold label for the sentiment example
        # is positive, i.e.,y=1
        y_train = np.array([
            [1],
        ])

        y_pred = logreg.predict_proba(
            X_train,
            weights,
            bias,
            fit_bias=fit_bias
        )

        print("y_pred = ", y_pred)
        # example a higher probability of being positive (.69)
        # than negative (.31)

        # loss for the first classifier should be (.37)
        cost = math_ops.log_loss(
            # calculate the loss on the training set
            y_train,
            y_pred
        )

        self.assertAlmostEqual(
            cost, 0.360985807
        )

        # suppose the correct gold label for the sentiment example
        # is negative, i.e.,y=0
        y_train = np.array([
            [0],
        ])

        # a higher probability of being positive (.69)
        # than negative (.31)

        # loss for the first classifier should be (.37)
        cost = math_ops.log_loss(
            # calculate the loss on the training set
            y_train,
            y_pred
        )

        self.assertAlmostEqual(
            cost, 1.19398580
        )
