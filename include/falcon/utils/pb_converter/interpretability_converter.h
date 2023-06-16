//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_INTERPRETABILITY_CONVERTER_H_
#define FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_INTERPRETABILITY_CONVERTER_H_

#include <string>

#include <falcon/inference/interpretability/lime/lime.h>

/**
 * serialize the LimeCompPredictionParams to string
 *
 * @param lime_sampling_params: LimeSamplingParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lime_sampling_params(
    const LimeSamplingParams &lime_sampling_params,
    std::string &output_message);

/**
 * deserialize LimeSamplingParams struct from an input string
 *
 * @param lime_sampling_params: deserialized LimeSamplingParams
 * @param input_message: serialized string
 */
void deserialize_lime_sampling_params(LimeSamplingParams &lime_sampling_params,
                                      const std::string &input_message);

/**
 * serialize the LimeCompPredictionParams to string
 *
 * @param lime_comp_pred_params: LimeCompPredictionParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lime_comp_pred_params(
    const LimeCompPredictionParams &lime_comp_pred_params,
    std::string &output_message);

/**
 * deserialize LimeCompPredictionParams struct from an input string
 *
 * @param lime_comp_pred_params: deserialized LimePreComputeParams
 * @param input_message: serialized string
 */
void deserialize_lime_comp_pred_params(
    LimeCompPredictionParams &lime_comp_pred_params,
    const std::string &input_message);

/**
 * serialize the LimeCompWeightsParams to string
 *
 * @param lime_comp_weights_params: LimeCompWeightsParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lime_comp_weights_params(
    const LimeCompWeightsParams &lime_comp_weights_params,
    std::string &output_message);

/**
 * deserialize LimeCompWeightsParams struct from an input string
 *
 * @param lime_comp_weights_params: deserialized LimeCompWeightsParams
 * @param input_message: serialized string
 */
void deserialize_lime_comp_weights_params(
    LimeCompWeightsParams &lime_comp_weights_params,
    const std::string &input_message);

/**
 * serialize the LimeFeatSelParams to string
 *
 * @param lime_feat_sel_params: LimeFeatSelParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lime_feat_sel_params(
    const LimeFeatSelParams &lime_feat_sel_params, std::string &output_message);

/**
 * deserialize LimeFeatSelParams struct from an input string
 *
 * @param lime_feat_sel_params: deserialized LimeFeatSelParams
 * @param input_message: serialized string
 */
void deserialize_lime_feat_sel_params(LimeFeatSelParams &lime_feat_sel_params,
                                      const std::string &input_message);

/**
 * serialize the LimeInterpretParams to string
 *
 * @param lime_interpret_params: LimeParams to be serialized
 * @param output_message: serialized string
 */
void serialize_lime_interpret_params(
    const LimeInterpretParams &lime_interpret_params,
    std::string &output_message);

/**
 * deserialize LimeInterpretParams struct from an input string
 *
 * @param lime_interpret_params: deserialized LimeInterpretParams
 * @param input_message: serialized string
 */
void deserialize_lime_interpret_params(
    LimeInterpretParams &lime_interpret_params,
    const std::string &input_message);

#endif // FALCON_INCLUDE_FALCON_UTILS_PB_CONVERTER_INTERPRETABILITY_CONVERTER_H_
