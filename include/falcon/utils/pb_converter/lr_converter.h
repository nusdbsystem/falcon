//
// Created by wuyuncheng on 20/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_LR_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_LR_CONVERTER_H_

#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <string>
#include <vector>

/**
 * serialize a lr model to string
 *
 * @param lr_model
 * @param output_str
 */
void serialize_lr_model(LinearModel lr_model, std::string &output_str);

/**
 * deserialize an input string to a lr_model
 *
 * @param lr_model
 * @param input_str
 */
void deserialize_lr_model(LinearModel &lr_model, std::string input_str);

#endif // FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_LR_CONVERTER_H_
