//
// Created by root on 11/17/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_BASE_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_BASE_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/party/party.h>
#include <vector>

// This class provides training of the explainable model, e.g., linear regression
class LimeBase {
 public:
  LimeBase() = default;
  ~LimeBase() = default;

 public:
  /**
   * This is the base explanation function given the plaintext neighborhood data,
   * ciphertext neighborhood labels, and ciphertext distances
   *
   * @param party: the participating party
   * @param plain_samples: sampled instances for a sample to be explained
   * @param cipher_probs: the ciphertext labels of the sampled instances
   *    for classification, the first dimension is sample number and
   *    the second dimension is the number of classes,
   *    for regression, the first dimension is always 1
   * @param cipher_weights: the ciphertext distances (weights) of the sampled instances
   * @param sample_size: the number of samples
   * @param feature_num: the number of features in the sample data
   * @param class_num: the number of classes for the original model, 1 for regression
   * @param label_id: the label id to be explained, specify the index in cipher_labels
   * @param num_selected_features: maximum number of features in explanation, reserved
   * @param feature_selection: how to select num_features, reserved.
   *    options in original lime include:
   *          'forward_selection': iteratively add features to the model.
   *            This is costly when num_features is high
   *          'highest_weights': selects the features that have the highest
   *                 product of absolute weight * original data point when
   *                 learning with all the features
   *          'lasso_path': chooses features based on the lasso
   *                 regularization path
   *          'none': uses all features, ignores num_features
   *          'auto': uses forward_selection if num_features <= 6, and
   *                 'highest_weights' otherwise.
   * @param model_regressor: regressor to use in explanation, default is Ridge linear regression
   * @param model_regressor_params: the parameters of the regressor
   */
  std::vector<double>  explain_instance_with_data(
      const Party& party,
      EncodedNumber** plain_samples,
      EncodedNumber** cipher_probs,
      EncodedNumber* cipher_weights,
      int sample_size,
      int feature_num,
      int class_num,
      int label_id,
      int num_selected_features,
      const std::string& feature_selection,
      const std::string& model_regressor,
      const std::string& model_regressor_params);

  /**
   * This function selects the feature indexes for training the interpret model,
   * currently, supported feature selection method is "pearson coefficient"
   *
   * @param party: the participating party
   * @param plain_samples: sampled instances for a sample to be explained
   * @param sample_labels: the sample labels for regression
   * @param cipher_weights: the sample weights
   * @param sample_size: the number of samples for training
   * @param feature_num: the number of total features (do not contain bias)
   * @param num_selected_features: the number of selected features
   * @param feature_selection: the feature selection method, "pearson"
   * @return
   */
  std::vector<int> select_features(
      const Party& party,
      EncodedNumber** plain_samples,
      EncodedNumber* sample_labels,
      EncodedNumber* cipher_weights,
      int sample_size,
      int feature_num,
      int num_selected_features,
      std::string feature_selection);

  /**
   * This function train an interpret model given the plain_data, cipher_labels,
   * cipher_weights, and the interpret model parameters
   *
   * @param party: the participating party
   * @param used_sampled_data: the selected plain samples
   * @param sample_labels: the cipher sample labels
   * @param cipher_weights: the cipher sample weights
   * @param sample_size: the number of samples
   * @param selected_feature_num: the selected feature number
   * @param model_regressor: the model regressor name, linear_regression or decision_tree
   * @param model_regressor_params: the model regressor params
   * @return
   */
  std::vector<double> train_interpret_model(
      const Party& party,
      EncodedNumber** used_sampled_data,
      EncodedNumber* sample_labels,
      EncodedNumber* cipher_weights,
      int sample_size,
      int selected_feature_num,
      std::string model_regressor,
      std::string model_regressor_params);
};

#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_BASE_H_
