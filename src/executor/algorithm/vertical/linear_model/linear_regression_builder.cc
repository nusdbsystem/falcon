//
// Created by root on 11/11/21.
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

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision
#include <utility>

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>

LinearRegressionBuilder::LinearRegressionBuilder() = default;

LinearRegressionBuilder::LinearRegressionBuilder(LinearRegressionParams linear_reg_params,
                                                 int m_weight_size,
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
  batch_size = linear_reg_params.batch_size;
  max_iteration = linear_reg_params.max_iteration;
  converge_threshold = linear_reg_params.converge_threshold;
  with_regularization = linear_reg_params.with_regularization;
  alpha = linear_reg_params.alpha;
  learning_rate = linear_reg_params.learning_rate;
  decay = linear_reg_params.decay;
  penalty = linear_reg_params.penalty;
  optimizer = linear_reg_params.optimizer;
  metric = linear_reg_params.metric;
  dp_budget = linear_reg_params.dp_budget;
  fit_bias = linear_reg_params.fit_bias;
  linear_reg_model = LinearRegressionModel(m_weight_size);
}

LinearRegressionBuilder::~LinearRegressionBuilder() = default;

void LinearRegressionBuilder::train(Party party) {

}

void LinearRegressionBuilder::distributed_train(const Party &party, const Worker &worker) {

}

void LinearRegressionBuilder::eval(Party party, falcon::DatasetType eval_type, const std::string &report_save_path) {

}


// initializes the LogisticRegressionBuilder instance
// and run .train() .eval() methods, then save model
void train_linear_regression(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train, Worker* worker) {
  log_info("Run train_logistic_regression");
  log_info("is_distributed_train = " + std::to_string(is_distributed_train));

  LinearRegressionParams params;
  deserialize_lir_params(params, params_str);
  log_linear_regression_params(params);

  // split train test data for party and populate the vectors
  std::vector<std::vector<double>> training_data;
  std::vector<std::vector<double>> testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    split_dataset(party, params.fit_bias, training_data, testing_data,
                  training_labels, testing_labels, split_percentage);
  } else {
    // TODO: add distributed train implementation
  }

  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;
  LinearRegressionBuilder linear_reg_builder(params,
                                             weight_size,
                                             training_data,
                                             testing_data,
                                             training_labels,
                                             testing_labels,
                                             training_accuracy,
                                             testing_accuracy);

  if (is_distributed_train == 0) {
    linear_reg_builder.train(*party);
    linear_reg_builder.eval(*party, falcon::TRAIN, model_report_file);
    linear_reg_builder.eval(*party, falcon::TEST, model_report_file);
    // save model and report
    auto* model_weights = new EncodedNumber[weight_size];
    std::string pb_lr_model_string;
    serialize_lr_model(linear_reg_builder.linear_reg_model, pb_lr_model_string);
    save_pb_model_string(pb_lr_model_string, model_save_file);
    // save_lr_model(log_reg_model_builder.log_reg_model, model_save_file);
    // save_training_report(log_reg_model.getter_training_accuracy(),
    //    log_reg_model.getter_testing_accuracy(),
    //    model_report_file);
    delete[] model_weights;
  } else {
    // TODO: add distributed train implementation
  }
}