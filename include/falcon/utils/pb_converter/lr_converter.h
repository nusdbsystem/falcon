//
// Created by wuyuncheng on 20/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_LR_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_LR_CONVERTER_H_

#include <vector>
#include <string>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>

/**
 * serialize a lr model to string
 *
 * @param lr_model
 * @param output_str
 */
void serialize_lr_model(LogisticRegressionModel lr_model, std::string & output_str);

/**
 * deserialize an input string to a lr_model
 *
 * @param lr_model
 * @param input_str
 */
void deserialize_lr_model(LogisticRegressionModel& lr_model, std::string input_str);


#endif //FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_LR_CONVERTER_H_
