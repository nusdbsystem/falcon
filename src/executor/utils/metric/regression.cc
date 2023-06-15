/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by root on 11/14/21.
//

#include <falcon/utils/logger/logger.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/utils/metric/regression.h>

#include <fstream>
#include <iomanip>  // std::setprecision
#include <iostream> // std::cout
#include <valarray>

RegressionMetrics::RegressionMetrics() = default;

void RegressionMetrics::compute_metrics(
    const std::vector<double> &predictions, const std::vector<double> &labels,
    const std::vector<double> &sample_weights) {
  if ((predictions.size() != labels.size()) ||
      (!sample_weights.empty() &&
       (predictions.size() != sample_weights.size()))) {
    log_error("The prediction size, label size, and sample_weights size"
              "are not equal");
  }
  // compute the metrics by calling the math functions
  mse = mean_squared_error(predictions, labels, sample_weights);
  rmse = sqrt(mse);
  msle = mean_squared_log_error(predictions, labels, sample_weights);
  rmsle = sqrt(msle);
  mae = median_absolute_error(predictions, labels);
  maxe = max_error(predictions, labels);
}

void RegressionMetrics::regression_report(std::ofstream &outfile) {
  if (!outfile) {
    // log to default
    log_info("=== Regression Report ===");
    log_info("MSE Loss = " + std::to_string(mse));
    log_info("RMSE Loss = " + std::to_string(rmse));
    log_info("MSLE Loss = " + std::to_string(msle));
    log_info("RMSLE Loss = " + std::to_string(rmsle));
    log_info("MAE Loss = " + std::to_string(mae));
    log_info("MAXE Loss = " + std::to_string(maxe));
  } else {
    outfile << "=== Regression Report ===\n";
    outfile << "MSE Loss = " << mse << "\n";
    outfile << "RMSE Loss = " << rmse << "\n";
    outfile << "MSLE Loss = " << msle << "\n";
    outfile << "RMSLE Loss = " << rmsle << "\n";
    outfile << "MAE Loss = " << mae << "\n";
    outfile << "MAXE Loss = " << maxe << "\n";
  }
}