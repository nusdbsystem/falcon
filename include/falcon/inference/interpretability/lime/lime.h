//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_

#include <string>
#include <vector>

struct LimeParams {
  // whether use precompute strategy
  bool is_precompute;
  // number of instances in the precompute stage
  int precompute_instance_num;
  // precompute saved file, to record the sampled instances and predictions
  std::string precompute_save_file;
  // type of model task, 'regression' or 'classification'
  std::string model_type;
  // number of classes in classification, set to 1 if regression
  int class_num;
  // if true, all non-categorical features will be discretized into quartiles
  bool discretize_continuous;
  // only matters if discretize_continuous is true, options are 'quartile',
  // 'decide', 'entropy', etc.
  std::string discretizer;
  // categorical feature ids
  std::vector<int> categorical_features;
  // kernel, similarity kernel that takes euclidean distances and kernel width
  // as input and outputs weights in (0,1). If not specified, default is exponential kernel
  std::string kernel;
  // width for the kernel
  double kernel_width;
  // feature selection method, reserved, options are 'forward_selection', 'lasso_path',
  // 'none' or 'auto', etc.
  std::string feature_selection;
  // whether sample around the predicting instance
  bool sample_around_instance;
  // sampling method, 'gaussian' or 'lhs'
  std::string sampling_method;
  // the number of sampled instances for explaining the prediction
  int sample_instance_num;
  // feature num at each party
  int local_feature_num;
  // feature num for the explanation
  int explained_feature_num;
  // interpretable method, 'linear_regression', 'decision_tree', etc.
  std::string interpretable_method;
  // params for the interpretable method, given base64 encoded string,
  // deserialize based on the interpretable method
  std::string interpretable_method_params;
};

#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_
