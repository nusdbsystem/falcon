//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_BUILDER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_BUILDER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_model.h>
#include <falcon/party/party.h>
#include <falcon/common.h>

#include <future>
#include <string>
#include <thread>
#include <vector>

struct LinearRegressionParams {
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
};

class LinearRegressionBuilder : public ModelBuilder {
 public:
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

 public:
  // linear regression model
  LinearRegressionModel linear_reg_model;

 public:
  /** default constructor */
  LinearRegressionBuilder();

  /**
   * linear regression constructor
   *
   * @param linear_reg_params: linear regression param structure
   * @param m_weight_size: number of weights
   * @param m_training_data: training data
   * @param m_testing_data: testing data
   * @param m_training_labels: training labels
   * @param m_testing_labels: testing labels
   * @param m_training_accuracy: training accuracy
   * @param m_testing_accuracy: testing accuracy
   */
  LinearRegressionBuilder(LinearRegressionParams linear_reg_params,
                          int m_weight_size,
                          std::vector< std::vector<double> > m_training_data,
                          std::vector< std::vector<double> > m_testing_data,
                          std::vector<double> m_training_labels,
                          std::vector<double> m_testing_labels,
                          double m_training_accuracy = 0.0,
                          double m_testing_accuracy = 0.0);

  /** destructor */
  ~LinearRegressionBuilder();

  /**
   * after receiving batch loss shares and truncated weight shares
   * from spdz parties, compute encrypted gradient
   *
   * @param party: initialized party object
   * @param batch_logistic_shares: secret shares of batch losses
   * @param batch_indexes: selected batch indexes
   * @param precision: precision for the batch samples and shares
   */
  void backward_computation(
      const Party& party,
      const std::vector<std::vector<double> >& batch_samples,
      EncodedNumber* predicted_labels,
      const std::vector<int>& batch_indexes,
      int precision,
      EncodedNumber* encrypted_gradients);

  /**
   * after receiving batch loss shares and truncated weight shares
   * from spdz parties, compute encrypted gradient
   *
   * @param party: initialized party object
   * @param batch_logistic_shares: secret shares of batch losses
   * @param batch_indexes: selected batch indexes
   * @param precision: precision for the batch samples and shares
   * @param ground_truth_labels: the encrypted ground truth labels
   * @param use_sample_weights: whether use sample weights
   * @param sample_weights: (encrypted) sample weights
   */
  void lime_backward_computation(
      const Party& party,
      const std::vector<std::vector<double> >& batch_samples,
      EncodedNumber* predicted_labels,
      const std::vector<int>& batch_indexes,
      int precision,
      EncodedNumber *ground_truth_labels,
      bool use_sample_weights,
      EncodedNumber *sample_weights,
      EncodedNumber* encrypted_gradients);

  /**
   * this function computes the regularized gradients of l1
   * regularization method, basically, it checks whether a weight
   * is larger than 0 or not, and assign the corresponding sign with
   * regularization hyper-parameter.
   *
   * @param party
   * @param regularized_gradients
   */
  void compute_l1_regularized_grad(const Party& party,
                                   EncodedNumber* regularized_gradients);

  /**
   * after receiving batch loss shares and truncated weight shares
   * from spdz parties, update the encrypted local weights
   *
   * @param party: initialized party object
   * @param encrypted_gradients: encrypted gradients
  */
  void update_encrypted_weights(Party& party, EncodedNumber* encrypted_gradients) const;

  /**
   * train a logistic regression model
   *
   * @param party: initialized party object
   */
  void train(Party party) override;

  /**
   * specific train function for lime
   *
   * @param party: initialized party object
   * @param use_encrypted_labels: whether use encrypted labels during training
   * @param encrypted_true_labels: encrypted labels used
   * @param use_sample_weights: whether use encrypted sample weights
   * @param encrypted_sample_weights: encrypted sample weights
   */
  void lime_train(Party party,
                  bool use_encrypted_labels,
                  EncodedNumber* encrypted_true_labels,
                  bool use_sample_weights,
                  EncodedNumber* encrypted_sample_weights);

  /**
   * train a logistic regression model
   *
   * @param party: initialized party object
   * @param worker: worker instance for distributed training
   */
  void distributed_train(const Party& party, const Worker& worker) override;

  /**
   * specific train function for lime
   *
   * @param party: initialized party object
   * @param worker: worker instance for distributed training
   * @param use_encrypted_labels: whether use encrypted labels during training
   * @param encrypted_true_labels: encrypted labels used
   * @param use_sample_weights: whether use encrypted sample weights
   * @param encrypted_sample_weights: encrypted sample weights
   */
  void distributed_lime_train(Party party,
                              const Worker& worker,
                              bool use_encrypted_labels,
                              EncodedNumber* encrypted_true_labels,
                              bool use_sample_weights,
                              EncodedNumber* encrypted_sample_weights);

  /**
   * evaluate a logistic regression model
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
   * compute the loss of the dataset in each iteration
   *
   * @param party: initialized party object
   * @param dataset_type: falcon::DatasetType,
   *   TRAIN for training data and TEST for testing data
   * @param loss: returned loss
   */
  double loss_computation(const Party& party, falcon::DatasetType dataset_type);

  /**
   * linear regression model eval
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

  /**
   * evaluate a linear regression model performance
   *
   * @param decrypted_labels: predicted labels
   * @param eval_type: falcon::DatasetType, TRAIN for training data and TEST for
   *   testing data will output both a pretty_print of confusion matrix
   *   as well as a classification metrics report
   * @param report_save_path: save the report into path
   */
  void eval_predictions_and_save(
      EncodedNumber* decrypted_labels,
      int sample_number,
      falcon::DatasetType eval_type,
      const std::string& report_save_path);
};

/**
 * spdz computation with thread,
 * the spdz_linear_regression_computation will do the l1 regularization
 *
 * @param party_num: number of parties
 * @param party_id: current party id
 * @param mpc_port_bases: port bases of the spdz parties
 * @param mpc_player_path: player data path of the spdz parties
 * @param party_host_names: spdz parties host names (ips)
 * @param batch_aggregation_shares: the batch shares
 * @param cur_batch_size: size of current batch
 * @param batch_loss_shares: promise structure of the loss shares
 */
void spdz_linear_regression_computation(int party_num,
                                        int party_id,
                                        std::vector<int> mpc_port_bases,
                                        const std::string& mpc_player_path,
                                        std::vector<std::string> party_host_names,
                                        const std::vector<double>& global_weights_shares,
                                        int global_weight_size,
                                        std::promise<std::vector<double>> *regularized_grad_shares);


#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_BUILDER_H_
