import numpy as np

class AverageMeter(object):
    """Computes and stores the average and current value"""

    def __init__(self):
        self.reset()

    def reset(self):
        self.val = 0
        self.avg = 0
        self.sum = 0
        self.count = 0

    def update(self, val, n=1):
        self.val = val
        self.sum += val * n
        self.count += n
        self.avg = self.sum / self.count

def eval_criterion(y, y_hat, metric='rmse'):
    if metric == 'rmse':
        return np.sqrt(((y_hat - y) ** 2).mean())
    elif metric == 'l2':
        return ((y_hat - y) ** 2).mean()
    elif metric == 'l1':
        return np.abs(y_hat-y).mean()