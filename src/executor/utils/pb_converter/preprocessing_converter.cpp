//
// Created by naili on 21/6/22.
//


#include "falcon/utils/pb_converter/preprocessing_converter.h"
#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include "../../include/message/pearson_feat_sel.pb.h"


void serialize_pearson_feat_sel_params(const PearsonFeatSelParams& pearson_feat_sel_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::PearsonFeatSelParams pb_pearson_feat_sel_params;
  pb_pearson_feat_sel_params.set_selected_samples_file(pearson_feat_sel_params.selected_samples_file);
  pb_pearson_feat_sel_params.set_selected_predictions_file(pearson_feat_sel_params.selected_predictions_file);
  pb_pearson_feat_sel_params.set_sample_weights_file(pearson_feat_sel_params.sample_weights_file);
  pb_pearson_feat_sel_params.set_num_samples(pearson_feat_sel_params.num_samples);
  pb_pearson_feat_sel_params.set_class_num(pearson_feat_sel_params.class_num);
  pb_pearson_feat_sel_params.set_class_id(pearson_feat_sel_params.class_id);
  pb_pearson_feat_sel_params.set_feature_selection(pearson_feat_sel_params.feature_selection);
  pb_pearson_feat_sel_params.set_num_explained_features(pearson_feat_sel_params.num_explained_features);
  pb_pearson_feat_sel_params.set_selected_features_file(pearson_feat_sel_params.selected_features_file);

  pb_pearson_feat_sel_params.SerializeToString(&output_message);
}

void deserialize_pearson_feat_sel_params(PearsonFeatSelParams& pearson_feat_sel_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::PearsonFeatSelParams pb_pearson_feat_sel_params;
  if (!pb_pearson_feat_sel_params.ParseFromString(input_message)) {
    log_error("Deserialize pearson feature selection params message failed.");
    exit(EXIT_FAILURE);
  }
  pearson_feat_sel_params.selected_samples_file = pb_pearson_feat_sel_params.selected_samples_file();
  pearson_feat_sel_params.selected_predictions_file = pb_pearson_feat_sel_params.selected_predictions_file();
  pearson_feat_sel_params.sample_weights_file = pb_pearson_feat_sel_params.sample_weights_file();
  pearson_feat_sel_params.num_samples = pb_pearson_feat_sel_params.num_samples();
  pearson_feat_sel_params.class_num = pb_pearson_feat_sel_params.class_num();
  pearson_feat_sel_params.class_id = pb_pearson_feat_sel_params.class_id();
  pearson_feat_sel_params.feature_selection = pb_pearson_feat_sel_params.feature_selection();
  pearson_feat_sel_params.num_explained_features = pb_pearson_feat_sel_params.num_explained_features();
  pearson_feat_sel_params.selected_features_file = pb_pearson_feat_sel_params.selected_features_file();
}