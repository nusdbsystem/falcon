//
// Created by root on 11/11/21.
//

#include <falcon/inference/interpretability/lime/lime.h>
#include <falcon/utils/pb_converter/interpretability_converter.h>
#include "../../include/message/interpretability.pb.h"

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

void serialize_lime_params(const LimeParams& lime_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::LimeParams pb_lime_params;
  pb_lime_params.set_is_precompute(lime_params.is_precompute);
  pb_lime_params.set_precompute_save_file(lime_params.precompute_save_file);
  pb_lime_params.set_model_type(lime_params.model_type);
  pb_lime_params.set_class_num(lime_params.class_num);
  pb_lime_params.set_discretize_continuous(lime_params.discretize_continuous);
  pb_lime_params.set_discretizer(lime_params.discretizer);
  for (int categorical_feature : lime_params.categorical_features) {
    pb_lime_params.add_categorical_features(categorical_feature);
  }
  pb_lime_params.set_kernel(lime_params.kernel);
  pb_lime_params.set_kernel_width(lime_params.kernel_width);
  pb_lime_params.set_feature_selection(lime_params.feature_selection);
  pb_lime_params.set_sample_around_instance(lime_params.sample_around_instance);
  pb_lime_params.set_sampling_method(lime_params.sampling_method);
  pb_lime_params.set_sample_instance_num(lime_params.sample_instance_num);
  pb_lime_params.set_local_feature_num(lime_params.local_feature_num);
  pb_lime_params.set_explained_feature_num(lime_params.explained_feature_num);
  pb_lime_params.set_interpretable_method(lime_params.interpretable_method);
  pb_lime_params.set_interpretable_method_params(lime_params.interpretable_method_params);

  pb_lime_params.SerializeToString(&output_message);
  // pb_lime_params.Clear();
}

void deserialize_lime_params(LimeParams& lime_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::LimeParams pb_lime_params;
  if (!pb_lime_params.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize lime params message failed.";
    return;
  }
  lime_params.is_precompute = pb_lime_params.is_precompute();
  lime_params.precompute_save_file = pb_lime_params.precompute_save_file();
  lime_params.model_type = pb_lime_params.model_type();
  lime_params.class_num = pb_lime_params.class_num();
  lime_params.discretize_continuous = pb_lime_params.discretize_continuous();
  lime_params.discretizer = pb_lime_params.discretizer();
  for (int i = 0; i < pb_lime_params.categorical_features_size(); i++) {
    lime_params.categorical_features.push_back(pb_lime_params.categorical_features(i));
  }
  lime_params.kernel = pb_lime_params.kernel();
  lime_params.kernel_width = pb_lime_params.kernel_width();
  lime_params.feature_selection = pb_lime_params.feature_selection();
  lime_params.sample_around_instance = pb_lime_params.sample_around_instance();
  lime_params.sampling_method = pb_lime_params.sampling_method();
  lime_params.sample_instance_num = pb_lime_params.sample_instance_num();
  lime_params.local_feature_num = pb_lime_params.local_feature_num();
  lime_params.explained_feature_num = pb_lime_params.explained_feature_num();
  lime_params.interpretable_method = pb_lime_params.interpretable_method();
  lime_params.interpretable_method_params = pb_lime_params.interpretable_method_params();
}