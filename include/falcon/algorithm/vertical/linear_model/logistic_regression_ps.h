//
// Created by nailixing on 03/07/21.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_PS_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_PS_H_


#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/model/model_io.h>
#include <falcon/common.h>

using namespace std;

class LRParameterServer:public ParameterServer{
 private:
  // LR model builder
  LogisticRegressionBuilder alg_builder;
  // party
  Party party;
 public:
  LRParameterServer(const LogisticRegressionBuilder& m_alg_builder,
      const Party& m_party,const std::string& ps_network_config_pb_str);

  LRParameterServer(const Party& m_party,const std::string& ps_network_config_pb_str);

  /**
   * copy constructor
   * @param log_reg_model
   */
  LRParameterServer(const LRParameterServer& obj);

  ~LRParameterServer();

 public:

  /**
  * send the split training data and testing data to workers
  *
  * @param training_data: split training dataset
  * @param testing_data: split testing dataset
  * @param training_labels: split training labels
  * @param testing_labels: split testing labels
  */
  void broadcast_train_test_data(const std::vector< std::vector<double> >& training_data,
                                 const std::vector< std::vector<double> >& testing_data,
                                 const std::vector<double>& training_labels,
                                 const std::vector<double>& testing_labels);

  /**
   * send the phe keys to workers
   */
  void broadcast_phe_keys();

  /**
   * distributed training process, partition data, collect result, update weights
   */
  void distributed_train() override;

  /**
   * distributed evaluation, partition data, collect result, compute evaluation matrix
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  void distributed_eval(falcon::DatasetType eval_type,
      const std::string& report_save_path) override;

  /**
   * distributed prediction, partition data, collect result, deserialize
   *
   * @param cur_test_data_indexes: vector of index
   * @param predicted_labels: return value, array of labels
   */
  void distributed_predict(
      const std::vector<int>& cur_test_data_indexes,
      EncodedNumber* predicted_labels) override;

  /**
   * save the trained model
   *
   * @param model_save_file: vector of index
   */
  void save_model(const std::string& model_save_file) override;

 private:

  /**
   * send encrypted weights to workers
   */
  void broadcast_encrypted_weights();

  /**
   * select batch indexes for each iteration
   *
   * @param party: initialized party object
   * @param data_indexes: the original training data indexes. need shuffle so pass by value
   * @return
   */
  std::vector<int> select_batch_idx(std::vector<int> data_indexes) const;

  /**
   * partition examples according to number of workers
   *
   * @param batch_indexes: batch's sample index
   */
  std::vector<int> partition_examples(std::vector<int> batch_indexes);

  /**
   * wait until receive all messages returned from worker
   *
   */
  std::vector<string> wait_worker_complete();

  /**
   * update weight
   *
   * @param encoded_messages: messages received from worker, this is loss in distributed training
   */
  EncodedNumber* update_encrypted_weights(const std::vector< string >& encoded_messages) const;
};

/**
 * run a master to help to train logistic regression
 *
 * @param party: init
 * @param params_str: LogisticRegressionParams serialized string
 * @param worker_address_str: worker's address 'ip+port'
 * @param model_save_file: the path for saving the trained model
 * @param model_report_file: the path for saving the training report
 */
void launch_lr_parameter_server(
    Party* party,
    const std::string& params_pb_str,
    const std::string& ps_network_config_pb_str,
    const std::string& model_save_file,
    const std::string& model_report_file);

#endif //FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_PS_H_
