//
// Created by root on 11/14/21.
//

#include <falcon/utils/metric/regression.h>
#include <falcon/utils/logger/logger.h>

#include <fstream>
#include <iomanip>   // std::setprecision
#include <iostream>  // std::cout

RegressionMetrics::RegressionMetrics() = default;

void RegressionMetrics::compute_metrics(std::vector<int> predictions,
                                        std::vector<double> labels,
                                        std::vector<double> sample_weights) {

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