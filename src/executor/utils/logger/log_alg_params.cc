//
// Created by root on 11/13/21.
//

#include <falcon/utils/logger/log_alg_params.h>

void log_linear_regression_params(const LinearRegressionParams& linear_reg_params) {
  log_info("linear_reg_params.batch_size = " + std::to_string(linear_reg_params.batch_size));
  log_info("linear_reg_params.max_iteration = " + std::to_string(linear_reg_params.max_iteration));
  log_info("linear_reg_params.converge_threshold = " + std::to_string(linear_reg_params.converge_threshold));
  log_info("linear_reg_params.with_regularization = " + std::to_string(linear_reg_params.with_regularization));
  log_info("linear_reg_params.alpha = " + std::to_string(linear_reg_params.alpha));
  log_info("linear_reg_params.learning_rate = " + std::to_string(linear_reg_params.learning_rate));
  log_info("linear_reg_params.decay = " + std::to_string(linear_reg_params.decay));
  log_info("linear_reg_params.penalty = " + linear_reg_params.penalty);
  log_info("linear_reg_params.optimizer = " + linear_reg_params.optimizer);
  log_info("linear_reg_params.metric = " + linear_reg_params.metric);
  log_info("linear_reg_params.dp_budget = " + std::to_string(linear_reg_params.dp_budget));
  log_info("linear_reg_params.fit_bias = " + std::to_string(linear_reg_params.fit_bias));
}

void log_logistic_regression_params(const LogisticRegressionParams& log_reg_params) {
  log_info("log_reg_params.batch_size = " + std::to_string(log_reg_params.batch_size));
  log_info("log_reg_params.max_iteration = " + std::to_string(log_reg_params.max_iteration));
  log_info("log_reg_params.converge_threshold = " + std::to_string(log_reg_params.converge_threshold));
  log_info("log_reg_params.with_regularization = " + std::to_string(log_reg_params.with_regularization));
  log_info("log_reg_params.alpha = " + std::to_string(log_reg_params.alpha));
  log_info("log_reg_params.learning_rate = " + std::to_string(log_reg_params.learning_rate));
  log_info("log_reg_params.decay = " + std::to_string(log_reg_params.decay));
  log_info("log_reg_params.penalty = " + log_reg_params.penalty);
  log_info("log_reg_params.optimizer = " + log_reg_params.optimizer);
  log_info("log_reg_params.multi_class = " + log_reg_params.multi_class);
  log_info("log_reg_params.metric = " + log_reg_params.metric);
  log_info("log_reg_params.dp_budget = " + std::to_string(log_reg_params.dp_budget));
  log_info("log_reg_params.fit_bias = " + std::to_string(log_reg_params.fit_bias));
}

void log_decision_tree_params(const DecisionTreeParams& dt_params) {
  log_info("dt_params.tree_type = " + dt_params.tree_type);
  log_info("dt_params.criterion = " + dt_params.criterion);
  log_info("dt_params.split_strategy = " + dt_params.split_strategy);
  log_info("dt_params.class_num = " + std::to_string(dt_params.class_num));
  log_info("dt_params.max_depth = " + std::to_string(dt_params.max_depth));
  log_info("dt_params.max_bins = " + std::to_string(dt_params.max_bins));
  log_info("dt_params.min_samples_split = " + std::to_string(dt_params.min_samples_split));
  log_info("dt_params.min_samples_leaf = " + std::to_string(dt_params.min_samples_leaf));
  log_info("dt_params.max_leaf_nodes = " + std::to_string(dt_params.max_leaf_nodes));
  log_info("dt_params.min_impurity_decrease = " + std::to_string(dt_params.min_impurity_decrease));
  log_info("dt_params.min_impurity_split = " + std::to_string(dt_params.min_impurity_split));
  log_info("dt_params.dp_budget = " + std::to_string(dt_params.dp_budget));
}

void log_random_forest_params(const RandomForestParams& random_forest_params) {
  log_info("random_forest_params.n_estimator = " + std::to_string(random_forest_params.n_estimator));
  log_info("random_forest_params.sample_rate = " + std::to_string(random_forest_params.sample_rate));
  log_info("random_forest_params.dt_param.tree_type = " + random_forest_params.dt_param.tree_type);
  log_info("random_forest_params.dt_param.criterion = " + random_forest_params.dt_param.criterion);
  log_info("random_forest_params.dt_param.split_strategy = " + random_forest_params.dt_param.split_strategy);
  log_info("random_forest_params.dt_param.class_num = " + std::to_string(random_forest_params.dt_param.class_num));
  log_info("random_forest_params.dt_param.max_depth = " + std::to_string(random_forest_params.dt_param.max_depth));
  log_info("random_forest_params.dt_param.max_bins = " + std::to_string(random_forest_params.dt_param.max_bins));
  log_info("random_forest_params.dt_param.min_samples_split = " + std::to_string(random_forest_params.dt_param.min_samples_split));
  log_info("random_forest_params.dt_param.min_samples_leaf = " + std::to_string(random_forest_params.dt_param.min_samples_leaf));
  log_info("random_forest_params.dt_param.max_leaf_nodes = " + std::to_string(random_forest_params.dt_param.max_leaf_nodes));
  log_info("random_forest_params.dt_param.min_impurity_decrease = " + std::to_string(random_forest_params.dt_param.min_impurity_decrease));
  log_info("random_forest_params.dt_param.min_impurity_split = " + std::to_string(random_forest_params.dt_param.min_impurity_split));
  log_info("random_forest_params.dt_param.dp_budget = " + std::to_string(random_forest_params.dt_param.dp_budget));
}

void log_gbdt_params(const GbdtParams& gbdt_params) {
  log_info("gbdt_params.n_estimator = " + std::to_string(gbdt_params.n_estimator));
  log_info("gbdt_params.loss = " + gbdt_params.loss);
  log_info("gbdt_params.learning_rate = " + std::to_string(gbdt_params.learning_rate));
  log_info("gbdt_params.subsample = " + std::to_string(gbdt_params.subsample));
  log_info("gbdt_params.dt_param.tree_type = " + gbdt_params.dt_param.tree_type);
  log_info("gbdt_params.dt_param.criterion = " + gbdt_params.dt_param.criterion);
  log_info("gbdt_params.dt_param.split_strategy = " + gbdt_params.dt_param.split_strategy);
  log_info("gbdt_params.dt_param.class_num = " + std::to_string(gbdt_params.dt_param.class_num));
  log_info("gbdt_params.dt_param.max_depth = " + std::to_string(gbdt_params.dt_param.max_depth));
  log_info("gbdt_params.dt_param.max_bins = " + std::to_string(gbdt_params.dt_param.max_bins));
  log_info("gbdt_params.dt_param.min_samples_split = " + std::to_string(gbdt_params.dt_param.min_samples_split));
  log_info("gbdt_params.dt_param.min_samples_leaf = " + std::to_string(gbdt_params.dt_param.min_samples_leaf));
  log_info("gbdt_params.dt_param.max_leaf_nodes = " + std::to_string(gbdt_params.dt_param.max_leaf_nodes));
  log_info("gbdt_params.dt_param.min_impurity_decrease = " + std::to_string(gbdt_params.dt_param.min_impurity_decrease));
  log_info("gbdt_params.dt_param.min_impurity_split = " + std::to_string(gbdt_params.dt_param.min_impurity_split));
  log_info("gbdt_params.dt_param.dp_budget = " + std::to_string(gbdt_params.dt_param.dp_budget));
}