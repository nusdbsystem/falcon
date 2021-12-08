//
// Created by wuyuncheng on 14/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/party/party.h>
#include <falcon/common.h>
#include <falcon/distributed/worker.h>
#include <falcon/utils/io_util.h>


#include <future>
#include <string>
#include <thread>
#include <vector>

struct LogisticRegressionParams {
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
  // strategy for handling multi-class classification, default 'ovr',
  // currently support 'ovr'
  // NOTE: ovr = one over rest
  std::string multi_class;
  // evaluation metric for training and testing, 'acc', 'auc', or 'ks',
  // currently support 'acc'
  std::string metric;
  // differential privacy (DP) budget, 0 denotes not use DP
  double dp_budget;
  // whether to fit the bias term
  bool fit_bias;
};

class LogisticRegressionBuilder : public ModelBuilder {
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
  // strategy for handling multi-class classification, default 'ovr',
  // currently support 'ovr'
  // NOTE: ovr = one over rest
  std::string multi_class;
  // evaluation metric for training and testing, 'acc', 'auc', or 'ks',
  // currently support 'acc'
  std::string metric;
  // differential privacy budget
  double dp_budget;
  // whether to fit the bias term
  bool fit_bias;

 public:
  // logistic regression model
  LogisticRegressionModel log_reg_model;

 public:
  /** default constructor */
  LogisticRegressionBuilder();

  /**
   * logistic regression constructor
   *
   * @param lr_params: logistic regression param structure
   * @param m_weight_size: number of weights
   * @param m_training_data: training data
   * @param m_testing_data: testing data
   * @param m_training_labels: training labels
   * @param m_testing_labels: testing labels
   * @param m_training_accuracy: training accuracy
   * @param m_testing_accuracy: testing accuracy
   */
  LogisticRegressionBuilder(LogisticRegressionParams lr_params,
      int m_weight_size,
      std::vector< std::vector<double> > m_training_data,
      std::vector< std::vector<double> > m_testing_data,
      std::vector<double> m_training_labels,
      std::vector<double> m_testing_labels,
      double m_training_accuracy = 0.0,
      double m_testing_accuracy = 0.0);

  /** destructor */
  ~LogisticRegressionBuilder();

  /**
   * initialize encrypted local weights
   *
   * @param party: initialized party object
   * @param precision: precision for big integer representation EncodedNumber
   */
  void init_encrypted_weights(const Party& party,
                              int precision = PHE_FIXED_POINT_PRECISION);

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
   * train a logistic regression model
   *
   * @param party: initialized party object
   * @param worker: worker instance for distributed training
   */
  void distributed_train(const Party& party, const Worker& worker) override;

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
   * logistic regression model eval
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
   * evaluate a logistic regression model
   *
   * @param decrypted_labels: predicted labels
   * @param eval_type: falcon::DatasetType, TRAIN for training data and TEST for
   *   testing data will output both a pretty_print of confusion matrix
   *   as well as a classification metrics report
   * @param report_save_path: save the report into path
   */
  void eval_matrix_computation_and_save(
      EncodedNumber* decrypted_labels,
      int sample_number,
      falcon::DatasetType eval_type,
      const std::string& report_save_path);

  /**
   * compute the loss of the dataset in each iteration
   *
   * @param party: initialized party object
   * @param dataset_type: falcon::DatasetType,
   *   TRAIN for training data and TEST for testing data
   * @param loss: returned loss
   */
  void loss_computation(Party party, falcon::DatasetType dataset_type,
                        double& loss);

  /**
   * print one ciphertext for debug
   *
   * @param party: initialized party object
   * @param number: ciphertext to be decrypted
   */
  void display_one_ciphertext(Party party, EncodedNumber *number);
};

/**
 * train a logistic regression model
 *
 * @param party: initialized party object
 * @param params_str: LogisticRegressionParams serialized string
 * @param model_save_file: saved model file
 * @param model_report_file: saved report file
 * @param is_distributed_train: 1: use distributed train
 * @param worker: worker instance, used when is_distributed_train=1
 */
void train_logistic_regression(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train=0, Worker* worker=nullptr);


#endif  // FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_H_
