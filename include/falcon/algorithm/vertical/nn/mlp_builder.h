//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_BUILDER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>
#include <falcon/algorithm/vertical/nn/mlp.h>

struct MlpParams {
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
  // the number of neurons in each layer
  std::vector<int> num_layers_neurons;
  // the vector of layers activation functions
  std::vector<std::string> layers_activation_funcs;
};

class MlpBuilder : public ModelBuilder {
 public:
  // size of mini-batch in each iteration
  int batch_size{};
  // maximum number of iterations for training
  int max_iteration{};
  // tolerance of convergence
  double converge_threshold{};
  // whether use regularization or not
  bool with_regularization{};
  // regularization parameter
  double alpha{};
  // learning rate for parameter updating
  double learning_rate{};
  // decay rate for learning rate, following lr = lr0 / (1 + decay*t),
  // t is #iteration
  double decay{};
  // penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
  std::string penalty;
  // optimization method, default 'sgd', currently support 'sgd'
  std::string optimizer;
  // evaluation metric for training and testing, 'mse'
  std::string metric;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget{};
  // whether to fit the bias term
  bool fit_bias{};
  // the number of neurons in each layer
  std::vector<int> num_layers_neurons;
  // the vector of layers activation functions
  std::vector<std::string> layers_activation_funcs;

 public:
  // the mlp model
  MlpModel mlp_model;
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_BUILDER_H_
