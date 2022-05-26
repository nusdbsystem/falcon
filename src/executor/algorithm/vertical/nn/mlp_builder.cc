//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/metric/classification.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/utils/metric/regression.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision
#include <utility>

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>

MlpBuilder::MlpBuilder() = default;

MlpBuilder::MlpBuilder(const MlpParams &mlp_params,
                       std::vector<std::vector<double>> m_training_data,
                       std::vector<std::vector<double>> m_testing_data,
                       std::vector<double> m_training_labels,
                       std::vector<double> m_testing_labels,
                       double m_training_accuracy,
                       double m_testing_accuracy)
                       : ModelBuilder(std::move(m_training_data),
                                      std::move(m_testing_data),
                                      std::move(m_training_labels),
                                      std::move(m_testing_labels),
                                      m_training_accuracy,
                                      m_testing_accuracy) {
  batch_size = mlp_params.batch_size;
  max_iteration = mlp_params.max_iteration;
  converge_threshold = mlp_params.converge_threshold;
  with_regularization = mlp_params.with_regularization;
  alpha = mlp_params.alpha;
  learning_rate = mlp_params.learning_rate;
  decay = mlp_params.decay;
  penalty = mlp_params.penalty;
  optimizer = mlp_params.optimizer;
  metric = mlp_params.metric;
  dp_budget = mlp_params.dp_budget;
  fit_bias = mlp_params.fit_bias;
  num_layers_neurons = mlp_params.num_layers_neurons;
  layers_activation_funcs = mlp_params.layers_activation_funcs;
  mlp_model = MlpModel(fit_bias, num_layers_neurons, layers_activation_funcs);
}

MlpBuilder::~MlpBuilder() {
  num_layers_neurons.clear();
  layers_activation_funcs.clear();
}

void MlpBuilder::train(Party party) {

}

void MlpBuilder::distributed_train(const Party &party, const Worker &worker) {

}

void MlpBuilder::eval(Party party, falcon::DatasetType eval_type,
                      const std::string &report_save_path) {

}

void MlpBuilder::distributed_eval(const Party &party, const Worker &worker,
                                  falcon::DatasetType eval_type) {

}