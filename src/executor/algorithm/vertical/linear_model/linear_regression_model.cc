//
// Created by root on 11/11/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/logger/logger.h>

#include <cmath>
#include <glog/logging.h>
#include <stack>
#include <Networking/ssl_sockets.h>
#include <iostream>

LinearRegressionModel::LinearRegressionModel() = default;

LinearRegressionModel::LinearRegressionModel(int m_weight_size) : LinearModel(m_weight_size) {}

LinearRegressionModel::~LinearRegressionModel() = default;

LinearRegressionModel::LinearRegressionModel(const LinearRegressionModel &linear_reg_model)
  : LinearModel(linear_reg_model) {}

LinearRegressionModel &LinearRegressionModel::operator=(const LinearRegressionModel &linear_reg_model) {
  LinearModel::operator=(linear_reg_model);
  return *this;
}

void LinearRegressionModel::predict(const Party &party,
                                    const std::vector<std::vector<double>> &predicted_samples,
                                    EncodedNumber *predicted_labels) const {

}

void LinearRegressionModel::forward_computation(const Party &party,
                                                int cur_batch_size,
                                                EncodedNumber **encoded_batch_samples,
                                                int &encrypted_batch_aggregation_precision,
                                                EncodedNumber *predicted_labels) const {

}