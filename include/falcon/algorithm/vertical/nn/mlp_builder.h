//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_BUILDER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>
#include <falcon/algorithm/vertical/nn/mlp.h>

struct MlpParams {
  // size of mini-batch in each iteration
  int batch_size;
  // maximum number of iterations for training
  int max_iteration;
  // tolerance of convergence
  double converge_threshold;
  // whether use regularization or not
  bool with_regularization;
  // regularization parameter
  double alpha;
  // learning rate for parameter updating
  double learning_rate;
  // decay rate for learning rate, following lr = lr0 / (1 + decay*t),
  // t is #iteration
  double decay;
  // penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
  std::string penalty;
  // optimization method, default 'sgd', currently support 'sgd'
  std::string optimizer;
  // evaluation metric for training and testing, 'mse'
  std::string metric;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget;
  // whether to fit the bias term
  bool fit_bias;
  // the number of neurons in each layer
  std::vector<int> num_layers_neurons;
  // the vector of layers activation functions
  std::vector<std::string> layers_activation_funcs;
};

class MlpBuilder : public ModelBuilder {
 public:
  // size of mini-batch in each iteration
  int batch_size{};
  // maximum number of iterations for training
  int max_iteration{};
  // tolerance of convergence
  double converge_threshold{};
  // whether use regularization or not
  bool with_regularization{};
  // regularization parameter
  double alpha{};
  // learning rate for parameter updating
  double learning_rate{};
  // decay rate for learning rate, following lr = lr0 / (1 + decay*t),
  // t is #iteration
  double decay{};
  // penalty method, default l2, currently only support 'l2'
  std::string penalty;
  // optimization method, default 'sgd', currently support 'sgd'
  std::string optimizer;
  // evaluation metric for training and testing, note that
  // the loss metric and activation function of the output layer
  // need to be matched:
  // (1) sigmoid and binary cross entropy;
  // (2) softmax and categorical cross entropy;
  // (3) identity with squared loss.
  std::string metric;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget{};
  // whether to fit the bias term
  bool fit_bias{};
  // the number of neurons in each layer
  std::vector<int> num_layers_neurons;
  // the vector of layers activation functions
  std::vector<std::string> layers_activation_funcs;

 public:
  // the mlp model
  MlpModel mlp_model;

 public:
  /** default constructor */
  MlpBuilder();

  /**
   * Mlp builder constructor
   *
   * @param mlp_params: the parameters of the mlp builder
   * @param m_training_data: training data
   * @param m_testing_data: testing data
   * @param m_training_labels: training labels
   * @param m_testing_labels: testing labels
   * @param m_training_accuracy: training accuracy
   * @param m_testing_accuracy: testing accuracy
   */
  MlpBuilder(const MlpParams& mlp_params,
             std::vector< std::vector<double> > m_training_data,
             std::vector< std::vector<double> > m_testing_data,
             std::vector<double> m_training_labels,
             std::vector<double> m_testing_labels,
             double m_training_accuracy = 0.0,
             double m_testing_accuracy = 0.0);

  /** destructor */
  ~MlpBuilder();

  /**
   * initialize encrypted local weights
   *
   * @param party: initialized party object
   * @param precision: precision for big integer representation EncodedNumber
   */
  void init_encrypted_weights(const Party& party,
                              int precision = PHE_FIXED_POINT_PRECISION);

  /**
   * compute the backward computation process
   *
   * @param party: initialized party object
   * @param batch_samples: the batch samples
   * @param predicted_labels: the predicted labels
   * @param batch_indexes: the selected batch indexes
   * @param precision: the precision for the batch samples
   * @param activation_shares: layers' activation shares of batch samples
   * @param deriv_activation_shares: layers' derivative activation shares of batch samples
   */
  void backward_computation(
      const Party& party,
      const std::vector<std::vector<double>>& batch_samples,
      EncodedNumber** predicted_labels,
      const std::vector<int>& batch_indexes,
      int precision,
      const TripleDVec& activation_shares,
      const TripleDVec& deriv_activation_shares);

  /**
   * compute the delta of the last layer (i.e., output layer)
   *
   * @param party: initialized party object
   * @param predicted_labels: the predicted labels
   * @param deltas: the deviations
   * @param batch_indexes: the last layer activation shares
   */
  void compute_last_layer_delta(
      const Party& party,
      EncodedNumber** predicted_labels,
      std::vector<EncodedNumber**>& deltas,
      const std::vector<int> &batch_indexes);

  /**
   * compute the gradients of a layer
   *
   * @param party: initialized party object
   * @param layer_idx: the index of the layer
   * @param sample_size: number of samples in a batch
   * @param activation_shares: the activation shares
   * @param deriv_activation_shares: the derivative activation shares
   * @param deltas: the deviations
   * @param weight_grads: the weight gradients of the layers
   * @param bias_grads: the bias gradients of the layers
   */
  void compute_loss_grad(
      const Party& party,
      int layer_idx,
      int sample_size,
      const TripleDVec& activation_shares,
      const TripleDVec& deriv_activation_shares,
      std::vector<EncodedNumber**>& deltas,
      std::vector<EncodedNumber**>& weight_grads,
      std::vector<EncodedNumber*>& bias_grads);

  /**
   * update the delta of a layer
   *
   * @param party: initialized party object
   * @param layer_idx: the index of the layer
   * @param sample_size: number of samples in a batch
   * @param activation_shares: the activation shares
   * @param deriv_activation_shares: the derivative activation shares
   * @param deltas: the deviations
   */
  void update_layer_delta(
      const Party& party,
      int layer_idx,
      int sample_size,
      const TripleDVec& activation_shares,
      const TripleDVec& deriv_activation_shares,
      std::vector<EncodedNumber**>& deltas);

  /**
   * layer-by-layer update weights
   *
   * @param party: initialized party object
   * @param weight_grads: the gradients of weights each neuron of each layer
   * @param bias_grads: the gradients of intercept in each neuron of each layer
   */
  void update_encrypted_weights(const Party& party,
                                const std::vector<EncodedNumber**>& weight_grads,
                                const std::vector<EncodedNumber*>& bias_grads);

  /**
   * train an mlp model
   *
   * @param party: initialized party object
   */
  void train(Party party) override;

  /**
   * train an mlp model
   *
   * @param party: initialized party object
   * @param worker: worker instance for distributed training
   */
  void distributed_train(const Party& party, const Worker& worker) override;

  /**
   * evaluate an mlp model
   *
   * @param party: initialized party object
   * @param eval_type: falcon::DatasetType, TRAIN for training data and TEST for
   *   testing data will output both a pretty_print of confusion matrix
   *   as well as a classification metrics report
   * @param report_save_path: save the report into path
   */
  void eval(Party party,
            falcon::DatasetType eval_type,
            const std::string& report_save_path = std::string()) override;

  /**
   * mlp model eval
   *
   * @param party: initialized party object
   * @param eval_type: falcon::DatasetType, TRAIN for training data and TEST for
   *   testing data will output both a pretty_print of confusion matrix
   *   as well as a classification metrics report
   * @param report_save_path: save the report into path
   */
  void distributed_eval(
      const Party &party,
      const Worker &worker,
      falcon::DatasetType eval_type);
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_BUILDER_H_
