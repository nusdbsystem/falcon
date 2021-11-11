//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_INTERPRETABILITY_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_INTERPRETABILITY_CONVERTER_H_

#include <string>

#include <falcon/inference/interpretability/lime/lime.h>

/**
 * serialize the LimeParams to string
 *
 * @param lime_params: LimeParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lime_params(const LimeParams& lime_params, std::string& output_message);

/**
 * deserialize logistic regression param struct from an input string
 *
 * @param lime_params: deserialized LimeParams
 * @param input_message: serialized string
 */
void deserialize_lime_params(LimeParams& lime_params, const std::string& input_message);

#endif //FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_INTERPRETABILITY_CONVERTER_H_
