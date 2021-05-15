//
// Created by wuyuncheng on 10/12/20.
//

#include "../../include/message/alg_params.pb.h"
#include <falcon/utils/pb_converter/alg_params_converter.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

void serialize_lr_params(LogisticRegressionParams lr_params, std::string& output_message) {
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
  logistic_regression_params.SerializeToString(&output_message);
}

void deserialize_lr_params(LogisticRegressionParams& lr_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LogisticRegressionParams logistic_regression_params;
  if (!logistic_regression_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize logistic regression params message failed.";
    return;
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
}

void serialize_dt_params(DecisionTreeParams dt_params, std::string& output_message) {
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
}

void deserialize_dt_params(DecisionTreeParams& dt_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::DecisionTreeParams decision_tree_params;
  if (!decision_tree_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize decision tree params message failed.";
    return;
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