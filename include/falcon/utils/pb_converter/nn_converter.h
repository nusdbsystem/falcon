//
// Created by root on 5/25/22.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NN_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NN_CONVERTER_H_

#include <falcon/algorithm/vertical/nn/mlp.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <string>

/**
 * serialize an mlp model to string
 *
 * @param mlp_model: the mlp model to be serialized
 * @param output_str: the output string
 */
void serialize_mlp_model(const MlpModel &mlp_model, std::string &output_str);

/**
 * deserialize an input string to an mlp model
 *
 * @param mlp_model: the mlp model to be deserialized and returned
 * @param input_str: the input string
 */
void deserialize_mlp_model(MlpModel &mlp_model, const std::string &input_str);

#endif // FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_NN_CONVERTER_H_
