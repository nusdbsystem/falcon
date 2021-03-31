import numpy as np


def sigmoid(logit):
    """
    The sigmoid takes a real-valued number and
    maps it into the range [0,1],
    which is good as probability
    """
    return 1.0 / (1 + np.exp(-logit))


def log_loss(y_true, y_pred, eps=1e-15):
    r"""
    modified from sklearn's log_loss
    Log loss, aka logistic loss or cross-entropy loss.
    the log loss is:
    .. math::
        L_{\log}(y, p) = -(y \log (p) + (1 - y) \log (1 - p))
    Parameters
    ----------
    y_true : array-like or label indicator matrix
        Ground truth (correct) labels for n_samples samples.
    y_pred : array-like of float, shape = (n_samples,)
        Predicted probabilities, as returned by a classifier's
        predict_proba method. If ``y_pred.shape = (n_samples,)``
        the probabilities provided are assumed to be that of the
        positive class. The labels in ``y_pred`` are assumed to be
        ordered alphabetically, as done by
        :class:`preprocessing.LabelBinarizer`.
    eps : float, default=1e-15
        Log loss is undefined for p=0 or p=1, so probabilities are
        clipped to max(eps, min(1 - eps, p)).
    Returns
    -------
    loss : float
    Notes
    -----
    The logarithm used is the natural logarithm (base-e).
    Examples
    --------
    # log_loss([1, 0, 0, 1],[0.9, 0.1, 0.2, 0.65])
    # = 0.21616187468057912
    """
    m = len(y_true)

    # Clipping
    # NOTE: if predictions == 0, then ln(0) = -inf
    # causing nan errors
    # use np.clip to clip min and max to near 0 or 1
    y_pred = np.clip(y_pred, eps, 1 - eps)

    positive_class_cost = y_true * np.log(y_pred)
    negative_class_cost = (1-y_true) * np.log(1-y_pred)

    total_cost = -(positive_class_cost + negative_class_cost)

    return total_cost.sum()/m
