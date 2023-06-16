//
// Created by naili on 21/6/22.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_PREPROCESSING_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_PREPROCESSING_CONVERTER_H_

#include "falcon/algorithm/vertical/preprocessing/pre_feature_selection.h"

/**
 * serialize the FeatSelParams to string
 *
 * @param lime_feat_sel_params: PearsonFeatSelParams to be serialized
 * @param output_message: serialized string
 */
void serialize_feat_sel_params(const FeatSelParams &feat_sel_params,
                               std::string &output_message);

/**
 * deserialize FeatSelParams struct from an input string
 *
 * @param lime_feat_sel_params: deserialized PearsonFeatSelParams
 * @param input_message: serialized string
 */
void deserialize_feat_sel_params(FeatSelParams &feat_sel_params,
                                 const std::string &input_message);

#endif // FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_PREPROCESSING_CONVERTER_H_
