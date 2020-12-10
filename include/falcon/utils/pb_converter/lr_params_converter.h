//
// Created by wuyuncheng on 10/12/20.
//

#ifndef FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_
#define FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_

#include <string>
#include <falcon/algorithm/vertical/linear_model/logistic_regression.h>

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

#endif //FALCON_SRC_EXECUTOR_UTILS_PB_CONVERTER_LR_PARAMS_CONVERTER_H_
