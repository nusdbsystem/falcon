//
// Created by root on 11/17/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_BASE_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_BASE_H_

#include <falcon/operator/phe/fixed_point_encoder.h>

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
   * @param neighborhood_plain_samples: sampled instances for a sample to be explained
   * @param neighborhood_cipher_labels: the ciphertext labels of the sampled instances
   *    for classification, the first dimension is the number of classes,
   *    for regression, the first dimension is always 1
   * @param neighborhood_cipher_distances: the ciphertext distances (weight) of the sampled instances
   * @param explain_label: the label to be explained, specify the index in neighborhood_cipher_labels
   * @param num_features: maximum number of features in explanation, reserved
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
  void explain_instance_with_data(EncodedNumber** neighborhood_plain_samples,
                                  EncodedNumber** neighborhood_cipher_labels,
                                  EncodedNumber* neighborhood_cipher_distances,
                                  int explain_label,
                                  int num_features,
                                  const std::string& feature_selection,
                                  const std::string& model_regressor,
                                  const std::string& model_regressor_params);


};

#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_BASE_H_
