//
// Created by root on 11/11/21.
//

#include <falcon/inference/interpretability/lime/lime.h>
#include <falcon/utils/pb_converter/interpretability_converter.h>
#include "../../include/message/interpretability.pb.h"

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

void serialize_lime_sampling_params(const LimeSamplingParams& lime_sampling_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LimeSamplingParams pb_lime_sampling_params;
  pb_lime_sampling_params.set_explain_instance_idx(lime_sampling_params.explain_instance_idx);
  pb_lime_sampling_params.set_sample_around_instance(lime_sampling_params.sample_around_instance);
  pb_lime_sampling_params.set_num_total_samples(lime_sampling_params.num_total_samples);
  pb_lime_sampling_params.set_sampling_method(lime_sampling_params.sampling_method);
  pb_lime_sampling_params.set_generated_sample_file(lime_sampling_params.generated_sample_file);

  pb_lime_sampling_params.SerializeToString(&output_message);
}

void deserialize_lime_sampling_params(LimeSamplingParams& lime_sampling_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LimeSamplingParams pb_lime_sampling_params;
  if (!pb_lime_sampling_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize lime sampling params message failed.";
    return;
  }
  lime_sampling_params.explain_instance_idx = pb_lime_sampling_params.explain_instance_idx();
  lime_sampling_params.sample_around_instance = pb_lime_sampling_params.sample_around_instance();
  lime_sampling_params.num_total_samples = pb_lime_sampling_params.num_total_samples();
  lime_sampling_params.sampling_method = pb_lime_sampling_params.sampling_method();
  lime_sampling_params.generated_sample_file = pb_lime_sampling_params.generated_sample_file();
}

void serialize_lime_comp_pred_params(const LimeCompPredictionParams& lime_comp_pred_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LimeCompPredictionParams pb_lime_comp_pred_params;
  pb_lime_comp_pred_params.set_original_model_name(lime_comp_pred_params.original_model_name);
  pb_lime_comp_pred_params.set_original_model_saved_file(lime_comp_pred_params.original_model_saved_file);
  pb_lime_comp_pred_params.set_generated_sample_file(lime_comp_pred_params.generated_sample_file);
  pb_lime_comp_pred_params.set_model_type(lime_comp_pred_params.model_type);
  pb_lime_comp_pred_params.set_class_num(lime_comp_pred_params.class_num);
  pb_lime_comp_pred_params.set_computed_prediction_file(lime_comp_pred_params.computed_prediction_file);

  pb_lime_comp_pred_params.SerializeToString(&output_message);
}

void deserialize_lime_comp_pred_params(LimeCompPredictionParams& lime_comp_pred_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LimeCompPredictionParams pb_lime_comp_pred_params;
  if (!pb_lime_comp_pred_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize lime comp_prediction params message failed.";
    return;
  }
  lime_comp_pred_params.original_model_name = pb_lime_comp_pred_params.original_model_name();
  lime_comp_pred_params.original_model_saved_file = pb_lime_comp_pred_params.original_model_saved_file();
  lime_comp_pred_params.model_type = pb_lime_comp_pred_params.model_type();
  lime_comp_pred_params.class_num = pb_lime_comp_pred_params.class_num();
  lime_comp_pred_params.generated_sample_file = pb_lime_comp_pred_params.generated_sample_file();
  lime_comp_pred_params.computed_prediction_file = pb_lime_comp_pred_params.computed_prediction_file();
}

void serialize_lime_comp_weights_params(const LimeCompWeightsParams& lime_comp_weights_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LimeCompWeightsParams pb_lime_comp_weights_params;
  pb_lime_comp_weights_params.set_explain_instance_idx(lime_comp_weights_params.explain_instance_idx);
  pb_lime_comp_weights_params.set_generated_sample_file(lime_comp_weights_params.generated_sample_file);
  pb_lime_comp_weights_params.set_computed_prediction_file(lime_comp_weights_params.computed_prediction_file);
  pb_lime_comp_weights_params.set_is_precompute(lime_comp_weights_params.is_precompute);
  pb_lime_comp_weights_params.set_num_samples(lime_comp_weights_params.num_samples);
  pb_lime_comp_weights_params.set_class_num(lime_comp_weights_params.class_num);
  pb_lime_comp_weights_params.set_distance_metric(lime_comp_weights_params.distance_metric);
  pb_lime_comp_weights_params.set_kernel(lime_comp_weights_params.kernel);
  pb_lime_comp_weights_params.set_kernel_width(lime_comp_weights_params.kernel_width);
  pb_lime_comp_weights_params.set_sample_weights_file(lime_comp_weights_params.sample_weights_file);
  pb_lime_comp_weights_params.set_selected_samples_file(lime_comp_weights_params.selected_samples_file);
  pb_lime_comp_weights_params.set_selected_predictions_file(lime_comp_weights_params.selected_predictions_file);

  pb_lime_comp_weights_params.SerializeToString(&output_message);
}

void deserialize_lime_comp_weights_params(LimeCompWeightsParams& lime_comp_weights_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LimeCompWeightsParams pb_lime_comp_weights_params;
  if (!pb_lime_comp_weights_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize lime comp_weights params message failed.";
    return;
  }
  lime_comp_weights_params.explain_instance_idx = pb_lime_comp_weights_params.explain_instance_idx();
  lime_comp_weights_params.generated_sample_file = pb_lime_comp_weights_params.generated_sample_file();
  lime_comp_weights_params.computed_prediction_file = pb_lime_comp_weights_params.computed_prediction_file();
  lime_comp_weights_params.is_precompute = pb_lime_comp_weights_params.is_precompute();
  lime_comp_weights_params.num_samples = pb_lime_comp_weights_params.num_samples();
  lime_comp_weights_params.class_num = pb_lime_comp_weights_params.class_num();
  lime_comp_weights_params.distance_metric = pb_lime_comp_weights_params.distance_metric();
  lime_comp_weights_params.kernel = pb_lime_comp_weights_params.kernel();
  lime_comp_weights_params.kernel_width = pb_lime_comp_weights_params.kernel_width();
  lime_comp_weights_params.sample_weights_file = pb_lime_comp_weights_params.sample_weights_file();
  lime_comp_weights_params.selected_samples_file = pb_lime_comp_weights_params.selected_samples_file();
  lime_comp_weights_params.selected_predictions_file = pb_lime_comp_weights_params.selected_predictions_file();
}

void serialize_lime_feat_sel_params(const LimeFeatSelParams& lime_feat_sel_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LimeFeatSelParams pb_lime_feat_sel_params;
  pb_lime_feat_sel_params.set_selected_samples_file(lime_feat_sel_params.selected_samples_file);
  pb_lime_feat_sel_params.set_selected_predictions_file(lime_feat_sel_params.selected_predictions_file);
  pb_lime_feat_sel_params.set_sample_weights_file(lime_feat_sel_params.sample_weights_file);
  pb_lime_feat_sel_params.set_num_samples(lime_feat_sel_params.num_samples);
  pb_lime_feat_sel_params.set_class_num(lime_feat_sel_params.class_num);
  pb_lime_feat_sel_params.set_class_id(lime_feat_sel_params.class_id);
  pb_lime_feat_sel_params.set_feature_selection(lime_feat_sel_params.feature_selection);
  pb_lime_feat_sel_params.set_num_explained_features(lime_feat_sel_params.num_explained_features);
  pb_lime_feat_sel_params.set_selected_features_file(lime_feat_sel_params.selected_features_file);

  pb_lime_feat_sel_params.SerializeToString(&output_message);
}

void deserialize_lime_feat_sel_params(LimeFeatSelParams& lime_feat_sel_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LimeFeatSelParams pb_lime_feat_sel_params;
  if (!pb_lime_feat_sel_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize lime feature selection params message failed.";
    return;
  }
  lime_feat_sel_params.selected_samples_file = pb_lime_feat_sel_params.selected_samples_file();
  lime_feat_sel_params.selected_predictions_file = pb_lime_feat_sel_params.selected_predictions_file();
  lime_feat_sel_params.sample_weights_file = pb_lime_feat_sel_params.sample_weights_file();
  lime_feat_sel_params.num_samples = pb_lime_feat_sel_params.num_samples();
  lime_feat_sel_params.class_num = pb_lime_feat_sel_params.class_num();
  lime_feat_sel_params.class_id = pb_lime_feat_sel_params.class_id();
  lime_feat_sel_params.feature_selection = pb_lime_feat_sel_params.feature_selection();
  lime_feat_sel_params.num_explained_features = pb_lime_feat_sel_params.num_explained_features();
  lime_feat_sel_params.selected_features_file = pb_lime_feat_sel_params.selected_features_file();
}

void serialize_lime_interpret_params(const LimeInterpretParams& lime_interpret_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LimeInterpretParams pb_lime_interpret_params;
  pb_lime_interpret_params.set_selected_data_file(lime_interpret_params.selected_data_file);
  pb_lime_interpret_params.set_selected_predictions_file(lime_interpret_params.selected_predictions_file);
  pb_lime_interpret_params.set_sample_weights_file(lime_interpret_params.sample_weights_file);
  pb_lime_interpret_params.set_num_samples(lime_interpret_params.num_samples);
  pb_lime_interpret_params.set_class_num(lime_interpret_params.class_num);
  pb_lime_interpret_params.set_class_id(lime_interpret_params.class_id);
  pb_lime_interpret_params.set_interpret_model_name(lime_interpret_params.interpret_model_name);
  pb_lime_interpret_params.set_interpret_model_param(lime_interpret_params.interpret_model_param);
  pb_lime_interpret_params.set_explanation_report(lime_interpret_params.explanation_report);

  pb_lime_interpret_params.SerializeToString(&output_message);
  // pb_lime_params.Clear();
}

void deserialize_lime_interpret_params(LimeInterpretParams& lime_interpret_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LimeInterpretParams pb_lime_interpret_params;
  if (!pb_lime_interpret_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize lime interpret params message failed.";
    return;
  }
  lime_interpret_params.selected_data_file = pb_lime_interpret_params.selected_data_file();
  lime_interpret_params.selected_predictions_file = pb_lime_interpret_params.selected_predictions_file();
  lime_interpret_params.sample_weights_file = pb_lime_interpret_params.sample_weights_file();
  lime_interpret_params.num_samples = pb_lime_interpret_params.num_samples();
  lime_interpret_params.class_num = pb_lime_interpret_params.class_num();
  lime_interpret_params.class_id = pb_lime_interpret_params.class_id();
  lime_interpret_params.interpret_model_name = pb_lime_interpret_params.interpret_model_name();
  lime_interpret_params.interpret_model_param = pb_lime_interpret_params.interpret_model_param();
  lime_interpret_params.explanation_report = pb_lime_interpret_params.explanation_report();
}