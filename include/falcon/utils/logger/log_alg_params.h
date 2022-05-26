//
// Created by root on 11/13/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_LOGGER_LOG_ALG_PARAMS_H_
#define FALCON_INCLUDE_FALCON_UTILS_LOGGER_LOG_ALG_PARAMS_H_

#include <falcon/utils/logger/logger.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>

/**
 * print the linear regression algorithm parameters
 */
void log_linear_regression_params(const LinearRegressionParams& linear_reg_params);

/**
 * print the logistic regression algorithm parameters
 */
void log_logistic_regression_params(const LogisticRegressionParams& log_reg_params);

/**
 * print the decision tree algorithm parameters
 */
void log_decision_tree_params(const DecisionTreeParams& dt_params);

/**
 * print the random forest algorithm parameters
 */
void log_random_forest_params(const RandomForestParams& random_forest_params);

/**
 * print the gradient boosting decision tree algorithm parameters
 */
void log_gbdt_params(const GbdtParams& gbdt_params);

/**
 * print the mlp algorithm parameters
 */
void log_mlp_params(const MlpParams& mlp_params);

#endif //FALCON_INCLUDE_FALCON_UTILS_LOGGER_LOG_ALG_PARAMS_H_
