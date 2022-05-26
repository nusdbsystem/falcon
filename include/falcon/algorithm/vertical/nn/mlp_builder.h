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
  // penalty method used, 'l1' or 'l2', default l2, currently support 'l2'
  std::string penalty;
  // optimization method, default 'sgd', currently support 'sgd'
  std::string optimizer;
  // evaluation metric for training and testing, 'mse'
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
