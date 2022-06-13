//
// Created by wuyuncheng on 10/12/20.
//

#include "../../include/message/alg_params.pb.h"
#include <falcon/utils/pb_converter/alg_params_converter.h>

#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include <google/protobuf/io/coded_stream.h>

void serialize_lr_params(const LogisticRegressionParams& lr_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LogisticRegressionParams logistic_regression_params;
  logistic_regression_params.set_batch_size(lr_params.batch_size);
  logistic_regression_params.set_max_iteration(lr_params.max_iteration);
  logistic_regression_params.set_converge_threshold(lr_params.converge_threshold);
  logistic_regression_params.set_with_regularization(lr_params.with_regularization);
  logistic_regression_params.set_alpha(lr_params.alpha);
  logistic_regression_params.set_learning_rate(lr_params.learning_rate);
  logistic_regression_params.set_decay(lr_params.decay);
  logistic_regression_params.set_penalty(lr_params.penalty);
  logistic_regression_params.set_optimizer(lr_params.optimizer);
  logistic_regression_params.set_multi_class(lr_params.multi_class);
  logistic_regression_params.set_metric(lr_params.metric);
  logistic_regression_params.set_differential_privacy_budget(lr_params.dp_budget);
  logistic_regression_params.set_fit_bias(lr_params.fit_bias);
  logistic_regression_params.SerializeToString(&output_message);
  logistic_regression_params.Clear();
}

void deserialize_lr_params(LogisticRegressionParams& lr_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LogisticRegressionParams logistic_regression_params;
  if (!logistic_regression_params.ParseFromString(input_message)) {
    log_error("Deserialize logistic regression params message failed.");
    exit(EXIT_FAILURE);
  }
  lr_params.batch_size = logistic_regression_params.batch_size();
  lr_params.max_iteration = logistic_regression_params.max_iteration();
  lr_params.converge_threshold = logistic_regression_params.converge_threshold();
  lr_params.with_regularization = logistic_regression_params.with_regularization();
  lr_params.alpha = logistic_regression_params.alpha();
  lr_params.learning_rate = logistic_regression_params.learning_rate();
  lr_params.decay = logistic_regression_params.decay();
  lr_params.penalty = logistic_regression_params.penalty();
  lr_params.optimizer = logistic_regression_params.optimizer();
  lr_params.multi_class = logistic_regression_params.multi_class();
  lr_params.metric = logistic_regression_params.metric();
  lr_params.dp_budget = logistic_regression_params.differential_privacy_budget();
  lr_params.fit_bias = logistic_regression_params.fit_bias();
}

void serialize_lir_params(const LinearRegressionParams& lir_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LinearRegressionParams linear_regression_params;
  linear_regression_params.set_batch_size(lir_params.batch_size);
  linear_regression_params.set_max_iteration(lir_params.max_iteration);
  linear_regression_params.set_converge_threshold(lir_params.converge_threshold);
  linear_regression_params.set_with_regularization(lir_params.with_regularization);
  linear_regression_params.set_alpha(lir_params.alpha);
  linear_regression_params.set_learning_rate(lir_params.learning_rate);
  linear_regression_params.set_decay(lir_params.decay);
  linear_regression_params.set_penalty(lir_params.penalty);
  linear_regression_params.set_optimizer(lir_params.optimizer);
  linear_regression_params.set_metric(lir_params.metric);
  linear_regression_params.set_differential_privacy_budget(lir_params.dp_budget);
  linear_regression_params.set_fit_bias(lir_params.fit_bias);
  linear_regression_params.SerializeToString(&output_message);
  linear_regression_params.Clear();
}

void deserialize_lir_params(LinearRegressionParams& lir_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LinearRegressionParams linear_regression_params;
  if (!linear_regression_params.ParseFromString(input_message)) {
    log_error("Deserialize linear regression params message failed.");
    exit(EXIT_FAILURE);
  }
  lir_params.batch_size = linear_regression_params.batch_size();
  lir_params.max_iteration = linear_regression_params.max_iteration();
  lir_params.converge_threshold = linear_regression_params.converge_threshold();
  lir_params.with_regularization = linear_regression_params.with_regularization();
  lir_params.alpha = linear_regression_params.alpha();
  lir_params.learning_rate = linear_regression_params.learning_rate();
  lir_params.decay = linear_regression_params.decay();
  lir_params.penalty = linear_regression_params.penalty();
  lir_params.optimizer = linear_regression_params.optimizer();
  lir_params.metric = linear_regression_params.metric();
  lir_params.dp_budget = linear_regression_params.differential_privacy_budget();
  lir_params.fit_bias = linear_regression_params.fit_bias();
}

void serialize_dt_params(const DecisionTreeParams& dt_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::DecisionTreeParams decision_tree_params;
  decision_tree_params.set_tree_type(dt_params.tree_type);
  decision_tree_params.set_criterion(dt_params.criterion);
  decision_tree_params.set_split_strategy(dt_params.split_strategy);
  decision_tree_params.set_class_num(dt_params.class_num);
  decision_tree_params.set_max_depth(dt_params.max_depth);
  decision_tree_params.set_max_bins(dt_params.max_bins);
  decision_tree_params.set_min_samples_split(dt_params.min_samples_split);
  decision_tree_params.set_min_samples_leaf(dt_params.min_samples_leaf);
  decision_tree_params.set_max_leaf_nodes(dt_params.max_leaf_nodes);
  decision_tree_params.set_min_impurity_decrease(dt_params.min_impurity_decrease);
  decision_tree_params.set_min_impurity_split(dt_params.min_impurity_split);
  decision_tree_params.set_dp_budget(dt_params.dp_budget);
  decision_tree_params.SerializeToString(&output_message);
  decision_tree_params.Clear();
}

void deserialize_dt_params(DecisionTreeParams& dt_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::DecisionTreeParams decision_tree_params;
  if (!decision_tree_params.ParseFromString(input_message)) {
    log_error("Deserialize decision tree params message failed.");
    exit(EXIT_FAILURE);
  }
  dt_params.tree_type = decision_tree_params.tree_type();
  dt_params.criterion = decision_tree_params.criterion();
  dt_params.split_strategy = decision_tree_params.split_strategy();
  dt_params.class_num = decision_tree_params.class_num();
  dt_params.max_depth = decision_tree_params.max_depth();
  dt_params.max_bins = decision_tree_params.max_bins();
  dt_params.min_samples_split = decision_tree_params.min_samples_split();
  dt_params.min_samples_leaf = decision_tree_params.min_samples_leaf();
  dt_params.max_leaf_nodes = decision_tree_params.max_leaf_nodes();
  dt_params.min_impurity_decrease = decision_tree_params.min_impurity_decrease();
  dt_params.min_impurity_split = decision_tree_params.min_impurity_split();
  dt_params.dp_budget = decision_tree_params.dp_budget();
}

void serialize_rf_params(const RandomForestParams& rf_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::RandomForestParams random_forest_params;
  random_forest_params.set_n_estimator(rf_params.n_estimator);
  random_forest_params.set_sample_rate(rf_params.sample_rate);
  auto *decision_tree_params = new com::nus::dbsytem::falcon::v0::DecisionTreeParams;
  decision_tree_params->set_tree_type(rf_params.dt_param.tree_type);
  decision_tree_params->set_criterion(rf_params.dt_param.criterion);
  decision_tree_params->set_split_strategy(rf_params.dt_param.split_strategy);
  decision_tree_params->set_class_num(rf_params.dt_param.class_num);
  decision_tree_params->set_max_depth(rf_params.dt_param.max_depth);
  decision_tree_params->set_max_bins(rf_params.dt_param.max_bins);
  decision_tree_params->set_min_samples_split(rf_params.dt_param.min_samples_split);
  decision_tree_params->set_min_samples_leaf(rf_params.dt_param.min_samples_leaf);
  decision_tree_params->set_max_leaf_nodes(rf_params.dt_param.max_leaf_nodes);
  decision_tree_params->set_min_impurity_decrease(rf_params.dt_param.min_impurity_decrease);
  decision_tree_params->set_min_impurity_split(rf_params.dt_param.min_impurity_split);
  decision_tree_params->set_dp_budget(rf_params.dt_param.dp_budget);
  random_forest_params.set_allocated_dt_param(decision_tree_params);
  random_forest_params.SerializeToString(&output_message);
  random_forest_params.Clear();
}

void deserialize_rf_params(RandomForestParams& rf_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::RandomForestParams random_forest_params;
  if (!random_forest_params.ParseFromString(input_message)) {
    log_error("Deserialize random forest params message failed.");
    exit(EXIT_FAILURE);
  }
  rf_params.n_estimator = random_forest_params.n_estimator();
  rf_params.sample_rate = random_forest_params.sample_rate();
  rf_params.dt_param.tree_type = random_forest_params.dt_param().tree_type();
  rf_params.dt_param.criterion = random_forest_params.dt_param().criterion();
  rf_params.dt_param.split_strategy = random_forest_params.dt_param().split_strategy();
  rf_params.dt_param.class_num = random_forest_params.dt_param().class_num();
  rf_params.dt_param.max_depth = random_forest_params.dt_param().max_depth();
  rf_params.dt_param.max_bins = random_forest_params.dt_param().max_bins();
  rf_params.dt_param.min_samples_split = random_forest_params.dt_param().min_samples_split();
  rf_params.dt_param.min_samples_leaf = random_forest_params.dt_param().min_samples_leaf();
  rf_params.dt_param.max_leaf_nodes = random_forest_params.dt_param().max_leaf_nodes();
  rf_params.dt_param.min_impurity_decrease = random_forest_params.dt_param().min_impurity_decrease();
  rf_params.dt_param.min_impurity_split = random_forest_params.dt_param().min_impurity_split();
  rf_params.dt_param.dp_budget = random_forest_params.dt_param().dp_budget();
}

void serialize_gbdt_params(const GbdtParams& gbdt_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::GbdtParams gradient_boosting_params;
  gradient_boosting_params.set_n_estimator(gbdt_params.n_estimator);
  gradient_boosting_params.set_loss(gbdt_params.loss);
  gradient_boosting_params.set_learning_rate(gbdt_params.learning_rate);
  gradient_boosting_params.set_subsample(gbdt_params.subsample);
  auto *decision_tree_params = new com::nus::dbsytem::falcon::v0::DecisionTreeParams;
  decision_tree_params->set_tree_type(gbdt_params.dt_param.tree_type);
  decision_tree_params->set_criterion(gbdt_params.dt_param.criterion);
  decision_tree_params->set_split_strategy(gbdt_params.dt_param.split_strategy);
  decision_tree_params->set_class_num(gbdt_params.dt_param.class_num);
  decision_tree_params->set_max_depth(gbdt_params.dt_param.max_depth);
  decision_tree_params->set_max_bins(gbdt_params.dt_param.max_bins);
  decision_tree_params->set_min_samples_split(gbdt_params.dt_param.min_samples_split);
  decision_tree_params->set_min_samples_leaf(gbdt_params.dt_param.min_samples_leaf);
  decision_tree_params->set_max_leaf_nodes(gbdt_params.dt_param.max_leaf_nodes);
  decision_tree_params->set_min_impurity_decrease(gbdt_params.dt_param.min_impurity_decrease);
  decision_tree_params->set_min_impurity_split(gbdt_params.dt_param.min_impurity_split);
  decision_tree_params->set_dp_budget(gbdt_params.dt_param.dp_budget);
  gradient_boosting_params.set_allocated_dt_param(decision_tree_params);
  gradient_boosting_params.SerializeToString(&output_message);
  gradient_boosting_params.Clear();
}

void deserialize_gbdt_params(GbdtParams& gbdt_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::GbdtParams gradient_boosting_params;
  if (!gradient_boosting_params.ParseFromString(input_message)) {
    log_error("Deserialize gradient boosting decision tree params message failed.");
    exit(EXIT_FAILURE);
  }
  gbdt_params.n_estimator = gradient_boosting_params.n_estimator();
  gbdt_params.loss = gradient_boosting_params.loss();
  gbdt_params.learning_rate = gradient_boosting_params.learning_rate();
  gbdt_params.subsample = gradient_boosting_params.subsample();
  gbdt_params.dt_param.tree_type = gradient_boosting_params.dt_param().tree_type();
  gbdt_params.dt_param.criterion = gradient_boosting_params.dt_param().criterion();
  gbdt_params.dt_param.split_strategy = gradient_boosting_params.dt_param().split_strategy();
  gbdt_params.dt_param.class_num = gradient_boosting_params.dt_param().class_num();
  gbdt_params.dt_param.max_depth = gradient_boosting_params.dt_param().max_depth();
  gbdt_params.dt_param.max_bins = gradient_boosting_params.dt_param().max_bins();
  gbdt_params.dt_param.min_samples_split = gradient_boosting_params.dt_param().min_samples_split();
  gbdt_params.dt_param.min_samples_leaf = gradient_boosting_params.dt_param().min_samples_leaf();
  gbdt_params.dt_param.max_leaf_nodes = gradient_boosting_params.dt_param().max_leaf_nodes();
  gbdt_params.dt_param.min_impurity_decrease = gradient_boosting_params.dt_param().min_impurity_decrease();
  gbdt_params.dt_param.min_impurity_split = gradient_boosting_params.dt_param().min_impurity_split();
  gbdt_params.dt_param.dp_budget = gradient_boosting_params.dt_param().dp_budget();
}

void serialize_mlp_params(const MlpParams& mlp_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::MlpParams pb_mlp_params;
  pb_mlp_params.set_is_classification(mlp_params.is_classification);
  pb_mlp_params.set_batch_size(mlp_params.batch_size);
  pb_mlp_params.set_max_iteration(mlp_params.max_iteration);
  pb_mlp_params.set_converge_threshold(mlp_params.converge_threshold);
  pb_mlp_params.set_with_regularization(mlp_params.with_regularization);
  pb_mlp_params.set_alpha(mlp_params.alpha);
  pb_mlp_params.set_learning_rate(mlp_params.learning_rate);
  pb_mlp_params.set_decay(mlp_params.decay);
  pb_mlp_params.set_penalty(mlp_params.penalty);
  pb_mlp_params.set_optimizer(mlp_params.optimizer);
  pb_mlp_params.set_metric(mlp_params.metric);
  pb_mlp_params.set_dp_budget(mlp_params.dp_budget);
  pb_mlp_params.set_fit_bias(mlp_params.fit_bias);
  int layer_size = (int) mlp_params.num_layers_outputs.size();
  int hidden_layer_size = (int) mlp_params.layers_activation_funcs.size();
  for (int i = 0; i < layer_size; i++) {
    pb_mlp_params.add_num_layers_outputs(mlp_params.num_layers_outputs[i]);
  }
  for (int i = 0; i < hidden_layer_size; i++) {
    pb_mlp_params.add_layers_activation_funcs(mlp_params.layers_activation_funcs[i]);
  }
  pb_mlp_params.SerializeToString(&output_message);
  pb_mlp_params.Clear();
}

void deserialize_mlp_params(MlpParams& mlp_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::MlpParams pb_mlp_params;
  if (!pb_mlp_params.ParseFromString(input_message)) {
    log_error("Deserialize mlp params message failed.");
    exit(EXIT_FAILURE);
  }
  mlp_params.is_classification = pb_mlp_params.is_classification();
  mlp_params.batch_size = pb_mlp_params.batch_size();
  mlp_params.max_iteration = pb_mlp_params.max_iteration();
  mlp_params.converge_threshold = pb_mlp_params.converge_threshold();
  mlp_params.with_regularization = pb_mlp_params.with_regularization();
  mlp_params.alpha = pb_mlp_params.alpha();
  mlp_params.learning_rate = pb_mlp_params.learning_rate();
  mlp_params.decay = pb_mlp_params.decay();
  mlp_params.penalty = pb_mlp_params.penalty();
  mlp_params.optimizer = pb_mlp_params.optimizer();
  mlp_params.metric = pb_mlp_params.metric();
  mlp_params.dp_budget = pb_mlp_params.dp_budget();
  mlp_params.fit_bias = pb_mlp_params.fit_bias();
  int layer_size = pb_mlp_params.num_layers_outputs_size();
  int hidden_layer_size = pb_mlp_params.layers_activation_funcs_size();
  for (int i = 0; i < layer_size; i++) {
    mlp_params.num_layers_outputs.push_back(pb_mlp_params.num_layers_outputs(i));
  }
  for (int i = 0; i < hidden_layer_size; i++) {
    mlp_params.layers_activation_funcs.push_back(pb_mlp_params.layers_activation_funcs(i));
  }
}
