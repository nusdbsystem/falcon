//
// Created by root on 11/14/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_METRIC_REGRESSION_H_
#define FALCON_INCLUDE_FALCON_UTILS_METRIC_REGRESSION_H_

#include <iostream>
#include <vector>

// a metrics class for regression performance
class RegressionMetrics {
  // inspired by scikit-learn's:
  // https://scikit-learn.org/stable/modules/model_evaluation.html
 public:
  // mean squared error = average of (Y-\hat{Y})^2
  double mse{};

  // root mean squared error = sqrt (mean_squared_error)
  double rmse{};

  // mean squared log error = average of log{(Y-\hat{Y})^2}
  double msle{};

  // root mean squared log error = sqrt (mean_squared_log_error)
  double rmsle{};

  // median absolute error = median of abs(Y-\hat{Y})
  double mae{};

  // max error = max of (Y - \hat{Y})
  double maxe{};

  // constructor
  RegressionMetrics();

  /**
   * compute the regression metrics
   *
   * @param predictions: the predicted labels
   * @param labels: the original labels
   * @param sample_weights: the weights of samples
   */
  void compute_metrics(std::vector<int> predictions,
                       std::vector<double> labels,
                       std::vector<double> sample_weights);

  // print the classifier performance report
  void regression_report(std::ofstream& outfile);
};

#endif //FALCON_INCLUDE_FALCON_UTILS_METRIC_REGRESSION_H_
