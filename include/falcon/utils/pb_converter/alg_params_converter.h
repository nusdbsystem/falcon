//
// Created by wuyuncheng on 10/12/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_

#include <string>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_builder.h>

/**
 * serialize the logistic regression param struct to string
 *
 * @param lr_params: LogisticRegressionParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lr_params(LogisticRegressionParams lr_params, std::string& output_message);

/**
 * deserialize logistic regression param struct from an input string
 *
 * @param lr_params: deserialized LogisticRegressionParams
 * @param input_message: serialized string
 */
void deserialize_lr_params(LogisticRegressionParams& lr_params, const std::string& input_message);

/**
 * serialize the decision tree param struct to string
 *
 * @param dt_params: DecisionTreeParams to be serialized
 * @param output_message: serialized string
 */
void serialize_dt_params(DecisionTreeParams dt_params, std::string& output_message);

/**
 * deserialize decision tree param struct from an input string
 *
 * @param dt_params: deserialized DecisionTreeParams
 * @param input_message: serialized string
 */
void deserialize_dt_params(DecisionTreeParams& dt_params, const std::string& input_message);

/**
 * serialize the random forest param struct to string
 *
 * @param rf_params: RandomForestParams to be serialized
 * @param output_message: serialized string
 */
void serialize_rf_params(RandomForestParams rf_params, std::string& output_message);

/**
 * deserialize random forest param struct from an input string
 *
 * @param rf_params: deserialized RandomForestParams
 * @param input_message: serialized string
 */
void deserialize_rf_params(RandomForestParams& rf_params, const std::string& input_message);

/**
 * serialize the gbdt param struct to string
 *
 * @param rf_params: GbdtParams to be serialized
 * @param output_message: serialized string
 */
void serialize_gbdt_params(GbdtParams gbdt_params, std::string& output_message);

/**
 * deserialize gbdt param struct from an input string
 *
 * @param rf_params: deserialized GbdtParams
 * @param input_message: serialized string
 */
void deserialize_gbdt_params(GbdtParams& gbdt_params, const std::string& input_message);

#endif //FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_
