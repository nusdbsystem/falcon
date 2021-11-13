//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_

#include <string>
#include <vector>

#include <falcon/common.h>
#include <falcon/party/party.h>
#include <falcon/operator/phe/fixed_point_encoder.h>

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

class LimeExplainer {
 public:
  // the lime params for the interpretability
  LimeParams params;

 public:
  /**
   * pre-compute the model predictions given the model type and model saved file
   *
   * @param party: the participating party
   * @param selected_samples: the samples randomly generated in the whole region
   * @param algorithm_name: the model name, e.g., RF, GBDT
   * @param model_saved_file: the file that saves the model
   */
  void precompute_predictions(
      const Party& party,
      const std::vector<std::vector<double>>& selected_samples,
      const falcon::AlgorithmName& algorithm_name,
      const std::string& model_saved_file);

  /**
   * sample a set of samples given the number of sample
   *
   * @param party: the participating party
   * @param sample_around_instance: whether sample around instance
   * @param data_row: if above is true, give the predicting sample
   * @param sample_instance_num: the number of samples needed to generate
   * @param sampling_method: the sampling method for generating the samples
   * @return
   */
  std::vector<std::vector<double>> generate_random_samples(
      const Party& party,
      bool sample_around_instance = false,
      const std::vector<double>& data_row = std::vector<double>(),
      int sample_instance_num = 5000,
      const std::string& sampling_method = "gaussian");

};

#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_
