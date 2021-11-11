//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_BUILDER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_model.h>
#include <falcon/party/party.h>
#include <falcon/common.h>

#include <future>
#include <string>
#include <thread>
#include <vector>

struct LinearRegressionParams {
  // size of mini-batch in each iteration
  int batch_size;
  // maximum number of iterations for training
  int max_iteration;
  // tolerance of convergence
  double converge_threshold;
  // whether use regularization or not
  bool with_regularization;
  // regularization parameter
  double alpha;
  // learning rate for parameter updating
  double learning_rate;
  // decay rate for learning rate, following lr = lr0 / (1 + decay*t),
  // t is #iteration
  double decay;
  // penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
  std::string penalty;
  // optimization method, default 'sgd', currently support 'sgd'
  std::string optimizer;
  // evaluation metric for training and testing, 'mse'
  std::string metric;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget;
  // whether to fit the bias term
  bool fit_bias;
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_BUILDER_H_
