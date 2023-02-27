//
// Created by wuyuncheng on 10/12/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_

#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>
#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <string>

/**
 * serialize the logistic regression param struct to string
 *
 * @param lr_params: LogisticRegressionParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lr_params(const LogisticRegressionParams &lr_params,
                         std::string &output_message);

/**
 * deserialize logistic regression param struct from an input string
 *
 * @param lr_params: deserialized LogisticRegressionParams
 * @param input_message: serialized string
 */
void deserialize_lr_params(LogisticRegressionParams &lr_params,
                           const std::string &input_message);

/**
 * serialize the linear regression param struct to string
 *
 * @param lr_params: LinearRegressionParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lir_params(const LinearRegressionParams &lir_params,
                          std::string &output_message);

/**
 * deserialize linear regression param struct from an input string
 *
 * @param lr_params: deserialized LinearRegressionParams
 * @param input_message: serialized string
 */
void deserialize_lir_params(LinearRegressionParams &lir_params,
                            const std::string &input_message);

/**
 * serialize the decision tree param struct to string
 *
 * @param dt_params: DecisionTreeParams to be serialized
 * @param output_message: serialized string
 */
void serialize_dt_params(const DecisionTreeParams &dt_params,
                         std::string &output_message);

/**
 * deserialize decision tree param struct from an input string
 *
 * @param dt_params: deserialized DecisionTreeParams
 * @param input_message: serialized string
 */
void deserialize_dt_params(DecisionTreeParams &dt_params,
                           const std::string &input_message);

/**
 * serialize the random forest param struct to string
 *
 * @param rf_params: RandomForestParams to be serialized
 * @param output_message: serialized string
 */
void serialize_rf_params(const RandomForestParams &rf_params,
                         std::string &output_message);

/**
 * deserialize random forest param struct from an input string
 *
 * @param rf_params: deserialized RandomForestParams
 * @param input_message: serialized string
 */
void deserialize_rf_params(RandomForestParams &rf_params,
                           const std::string &input_message);

/**
 * serialize the gbdt param struct to string
 *
 * @param gbdt_params: GbdtParams to be serialized
 * @param output_message: serialized string
 */
void serialize_gbdt_params(const GbdtParams &gbdt_params,
                           std::string &output_message);

/**
 * deserialize gbdt param struct from an input string
 *
 * @param gbdt_params: deserialized GbdtParams
 * @param input_message: serialized string
 */
void deserialize_gbdt_params(GbdtParams &gbdt_params,
                             const std::string &input_message);

/**
 * serialize the mlp param struct to string
 *
 * @param mlp_params: MlpParams to be serialized
 * @param output_message: serialized string
 */
void serialize_mlp_params(const MlpParams &mlp_params,
                          std::string &output_message);

/**
 * deserialize mlp param struct from an input string
 *
 * @param mlp_params: deserialized MlpParams
 * @param input_message: serialized string
 */
void deserialize_mlp_params(MlpParams &mlp_params,
                            const std::string &input_message);

#endif // FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_
