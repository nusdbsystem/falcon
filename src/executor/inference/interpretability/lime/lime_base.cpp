//
// Created by root on 12/20/21.
//

#include <falcon/inference/interpretability/lime/lime_base.h>
#include <falcon/utils/logger/logger.h>

std::vector<double> LimeBase::explain_instance_with_data(
    const Party& party,
    EncodedNumber **plain_samples,
    EncodedNumber **cipher_probs,
    EncodedNumber *cipher_weights,
    int sample_size,
    int feature_num,
    int class_num,
    int label_id,
    int num_selected_features,
    const std::string &feature_selection,
    const std::string &model_regressor,
    const std::string &model_regressor_params) {
  // To explain given the plain_samples, cipher_labels, cipher_distances, and explain_label
  //  we do the following steps:
  //    1. compute the sample weights given kernel function
  //    2. if `feature_selection != none` and `num_features < sampled_data[0].size()`
  //        execute the feature selection process, now support only pearson and Ridge
  //        with the highest weights
  //    3. use the result to fit a regressor model
  //    4. return the regression model weights

//  //    step 1. compute the sample weights given kernel function
//  EncodedNumber* sample_weights = compute_kernel_weights(cipher_distances);
//  LOG(INFO) << "[explain_instance_with_data] Compute sample weights finished.";

  //    step 2. select features for using in the linear regression model
  auto* sample_labels = new EncodedNumber[sample_size];
  for (int i = 0; i < sample_size; i++) {
    sample_labels[i] = cipher_probs[i][label_id];
  }
  std::vector<int> selected_feature_idx = select_features(
      party,
      plain_samples,
      sample_labels,
      cipher_weights,
      sample_size,
      feature_num,
      num_selected_features,
      feature_selection);
  LOG(INFO) << "[explain_instance_with_data] Select features finished.";
  LOG(INFO) << "[explain_instance_with_data] selected_feature_idx.size = " << selected_feature_idx.size();
  LOG(INFO) << "[explain_instance_with_data] selected_feature_idx[0] = " << selected_feature_idx[0];
  google::FlushLogFiles(google::INFO);

  //    step 3. use the result to fit a linear regression model
  int selected_feature_num = (int) selected_feature_idx.size();
  auto** used_sampled_data = new EncodedNumber*[sample_size];
  for (int i = 0; i < sample_size; i++) {
    used_sampled_data[i] = new EncodedNumber[selected_feature_num];
  }
  for (int i = 0; i < sample_size; i++) {
    for (int j = 0; j < selected_feature_num; j++) {
      used_sampled_data[i][j] = plain_samples[i][selected_feature_idx[j]];
    }
  }
  std::vector<double> model_weights = train_interpret_model(
      party,
      used_sampled_data,
      sample_labels,
      cipher_weights,
      sample_size,
      selected_feature_num,
      model_regressor,
      model_regressor_params
  );
  LOG(INFO) << "[explain_instance_with_data] Train interpret model finished.";
  LOG(INFO) << "[explain_instance_with_data] model_weights.size = " << model_weights.size();
  LOG(INFO) << "[explain_instance_with_data] model_weights[0] = " << model_weights[0];

  for (int j = 0; j < selected_feature_num; j++) {
    delete used_sampled_data[j];
  }
  delete [] used_sampled_data;

  //    step 4. return the regression model weights
  return model_weights;
}

std::vector<int> LimeBase::select_features(const Party& party,
                                           EncodedNumber **plain_samples,
                                           EncodedNumber *sample_labels,
                                           EncodedNumber *cipher_weights,
                                           int sample_size,
                                           int feature_num,
                                           int num_selected_features,
                                           std::string feature_selection) {
  std::vector<int> local_feature_idx;

  return local_feature_idx;
}

std::vector<double> LimeBase::train_interpret_model(const Party& party,
                                                    EncodedNumber **used_sampled_data,
                                                    EncodedNumber *sample_labels,
                                                    EncodedNumber *cipher_weights,
                                                    int sample_size,
                                                    int selected_feature_num,
                                                    std::string model_regressor,
                                                    std::string model_regressor_params) {
  std::vector<double> explanation;


  return explanation;
}