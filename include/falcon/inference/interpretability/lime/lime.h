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
#include "scaler.h"

struct LimeCompPredictionParams {
  // vertical original model name
  std::string original_model_name;
  // vertical original model saved file
  std::string original_model_saved_file;
  // type of model task, 'regression' or 'classification'
  std::string model_type;
  // number of classes in classification, set to 1 if regression
  int class_num;
  // the instance index for explain
  int explain_instance_idx;
  // whether sampling around the above instance
  bool sample_around_instance;
  // number of total samples to be generated
  int num_total_samples;
  // the sampling method, now only support "gaussian"
  std::string sampling_method;
  // generated samples save file
  std::string generated_sample_file;
  // prediction save file
  std::string computed_prediction_file;
};

struct LimeCompWeightsParams {
  // the instance index for explain
  int explain_instance_idx;
  // generated samples save file
  string generated_sample_file;
  // prediction save file
  string computed_prediction_file;
  // whether it is pre-computed
  bool is_precompute;
  // number of samples to be generated or selected
  int num_samples;
  // number of classes in classification, set to 1 if regression
  int class_num;
  // the metric for computing the distance, only "euclidean"
  string distance_metric;
  // kernel, similarity kernel that takes euclidean distances and kernel width
  // as input and outputs weights in (0,1). If not specified, default is exponential kernel
  string kernel;
  // width for the kernel
  double kernel_width;
  // sample weights file to be saved
  string sample_weights_file;
  // selected samples to be saved if is_precompute = true
  string selected_samples_file;
  // selected predictions to be saved if is_precompute = true
  string selected_predictions_file;
};

struct LimeFeatSelParams {
  // selected samples file
  string selected_samples_file;
  // selected predictions file
  string selected_predictions_file;
  // the sample weights file
  string sample_weights_file;
  // number of samples generated or selected
  int num_samples;
  // number of classes in classification, set to 1 if regression
  int class_num;
  // the label id to be explained
  int class_id;
  // feature selection method, current options are 'pearson', 'lasso_path',
  string feature_selection;
  // number of features to be explained in the interpret model
  int num_explained_features;
  // selected features to be saved
  string selected_features_file;
};

struct LimeInterpretParams {
  // selected data file, either selected_samples_file or selected_features_file
  string selected_data_file;
  // selected predictions saved
  string selected_predictions_file;
  // sample weights file saved
  string sample_weights_file;
  // number of samples generated or selected
  int num_samples;
  // number of classes in classification, set to 1 if regression
  int class_num;
  // the label id to be explained
  int class_id;
  // interpretable model name, linear_regression or decision_tree
  string interpret_model_name;
  // interpretable model params, should be serialized LinearRegressionParams or DecisionTreeParams
  string interpret_model_param;
  // explanation report
  string explanation_report;
};

class LimeExplainer {
 public:
  LimeExplainer() = default;
  ~LimeExplainer() = default;

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
      StandardScaler* scaler,
      bool sample_around_instance = false,
      const std::vector<double>& data_row = std::vector<double>(),
      int sample_instance_num = 5000,
      const std::string& sampling_method = "gaussian");

  /**
   * load the vertical federated learning model and predict
   *
   * @param party: the participating party
   * @param origin_model_name: the original model name
   * @param origin_model_saved_file: the model pb string saved path
   * @param generated_samples: the generated random samples
   * @param model_type: the model type, either regression or classification
   * @param class_num: the number of classes
   * @param predictions: the returned ciphertext predictions
   */
  void load_predict_origin_model(
      const Party& party,
      const std::string& origin_model_name,
      const std::string& origin_model_saved_file,
      std::vector<std::vector<double>> generated_samples,
      std::string model_type,
      int class_num,
      EncodedNumber** predictions);

  /**
   * This function computes the encrypted sample weights and save
   *
   * @param party: the participating party
   * @param generated_sample_file: the generated sample file in the first step
   * @param computed_prediction_file: the computed prediction file in the first step
   * @param is_precompute: whether precompute is enabled
   * @param num_samples: the number of samples for training the interpret model
   * @param class_num: classification: equal to class_num; regression: 1
   * @param distance_metric: the metric for evaluating the distance between
   *    origin_data and sampled_data
   * @param kernel: the kernel function for computing the weights
   * @param kernel_width: the kernel width, can use default value
   * @param sample_weights_file: the sample weights file to be saved
   * @param selected_sample_file: the selected sample file to be saved
   * @param selected_prediction_file: the selected prediction file to be saved
   */
  void compute_sample_weights(const Party& party,
                               const std::string& generated_sample_file,
                               const std::string& computed_prediction_file,
                               bool is_precompute,
                               int num_samples,
                               int class_num,
                               const std::string& distance_metric,
                               const std::string& kernel,
                               double kernel_width,
                               const std::string& sample_weights_file,
                               const std::string& selected_sample_file,
                               const std::string& selected_prediction_file);

  /**
   * randomly select a set of sample indexes
   *
   * @param party: the participating party
   * @param num_total_samples: the total number of samples
   * @param num_samples: the number of selected samples
   * @return
   */
  static std::vector<int> random_select_sample_idx(const Party& party,
                                            int generated_samples_size,
                                            int num_samples);

  /**
   * This function computes the distance between origin data
   * and then compute the exponential kernel weights
   *
   * @param party: the participating party
   * @param weights: the returned encrypted weights
   * @param origin_data: the data to be explained
   * @param sampled_data: the sampled data
   * @param distance_metric: the distance metric between data samples, "euclidean"
   * @param kernel: the kernel function of the distance, "exponential"
   * @param kernel_width: the kernel width for the kernel function
   * @return
   */
  void compute_dist_weights(
      const Party& party,
      EncodedNumber* weights,
      const std::vector<double>& origin_data,
      const std::vector<std::vector<double>>& sampled_data,
      const std::string& distance_metric,
      const std::string& kernel,
      double kernel_width);

  /**
   * This function computes the squared distance between origin_data
   * and sampled_data across all the parties
   *
   * @param party: the participating party
   * @param origin_data: the data to be explained
   * @param sampled_data: the sampled data
   * @param squared_dist: the returned squared distances
   */
  void compute_squared_dist(const Party& party,
                            const std::vector<double>& origin_data,
                            const std::vector<std::vector<double>>& sampled_data,
                            EncodedNumber* squared_dist);

  /**
   * This function selects num_explained_features for training
   * the interpret model
   *
   * @param party: the participating party
   * @param selected_sample_file: the selected sample file to be saved
   * @param selected_prediction_file: the selected prediction file to be saved
   * @param sample_weights_file: the encrypted sample weights file
   * @param num_samples: the number of selected samples
   * @param class_num: the number of classes
   * @param class_id: the class_id to be explained
   * @param feature_selection: selection method, currently only support pearson
   * @param num_explained_features: the number of features to be explained
   * @param selected_features_file: the selected features file to be saved
   */
  void select_features(Party party,
                       const std::string& selected_samples_file,
                       const std::string& selected_predictions_file,
                       const std::string &sample_weights_file,
                       int num_samples,
                       int class_num,
                       int class_id,
                       const std::string& feature_selection,
                       int num_explained_features,
                       const std::string& selected_features_file);

  /**
   * This function explains a origin_data's model prediction
   *
   * @param party: the participating party
   * @param selected_features_file: the data for training the interpret model
   * @param selected_predictions_file: the predictions of the above data
   * @param sample_weights_file: the sample weights file for the training
   * @param num_samples: the number of samples for training the interpret model
   * @param class_num: classification: equal to class_num; regression: 1
   * @param class_id: the label id to be explained, if given
   * @param interpret_model_name: the interpret model name,
   *    linear regression or decision tree
   * @param interpret_model_param: the parameters of the model, now use json
   * @param explanation_report: the report for save the explanations
   * @return
   */
  std::vector<double> interpret(
      const Party& party,
      const std::string& selected_data_file,
      const std::string& selected_predictions_file,
      const std::string& sample_weights_file,
      int num_samples,
      int class_num,
      int class_id,
      const std::string& interpret_model_name,
      const std::string& interpret_model_param,
      const std::string& explanation_report);

  /**
   * This function explain a specific label by training a model
   *
   * @param party: the participating party
   * @param train_data: the plaintext train data
   * @param predictions: the encrypted model predictions
   * @param sample_weights: the encrypted sample weights
   * @param num_samples: the number of samples for training
   * @param interpret_model_name: the model to be trained
   * @param interpret_model_param: the model params to be used for training
   * @return
   */
  std::vector<double> explain_one_label(
      const Party &party,
      std::vector<std::vector<double>> train_data,
      EncodedNumber* predictions,
      EncodedNumber* sample_weights,
      int num_samples,
      const std::string &interpret_model_name,
      const std::string &interpret_model_param);
};

/**
 * pre-generate the samples and compute the model predictions
 *
 * @param party: the participating party
 * @param params_str: the algorithm params, aka. LimeCompPredictionParams
 */
void lime_comp_pred(Party party, const std::string& params_str);

/**
 * compute the sample weights
 *
 * @param party: the participating party
 * @param params_str: the algorithm params, aka. LimeCompWeightsParams
 */
void lime_comp_weight(Party party, const std::string& params_str);

/**
 * select the features (optional)
 *
 * @param party: the participating party
 * @param params_str: the algorithm params, aka. LimeFeatureSelectionParams
 */
void lime_feat_sel(Party party, const std::string& params_str);

/**
 * interpret a given sample
 *
 * @param party: the participating party
 * @param params_str: the algorithm params, aka. LimeInterpretParams
 */
void lime_interpret(Party party, const std::string& params_str);

/**
 * spdz computation with thread,
 * the spdz_lime_computation will do the sqrt(dist), kernel
 * exponential, and pearson coefficient operation
 *
 * @param party_num
 * @param party_id
 * @param mpc_tree_port_bases
 * @param party_host_names
 * @param public_value_size
 * @param public_values
 * @param private_value_size
 * @param private_values
 * @param tree_comp_type
 * @param res
 */
void spdz_lime_computation(int party_num,
                           int party_id,
                           std::vector<int> mpc_port_bases,
                           std::vector<std::string> party_host_names,
                           int public_value_size,
                           const std::vector<int>& public_values,
                           int private_value_size,
                           const std::vector<double>& private_values,
                           falcon::SpdzLimeCompType lime_comp_type,
                           std::promise<std::vector<double>> *res);


#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_H_
