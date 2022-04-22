//
// Created by root on 11/17/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_PS_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_PS_H_

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/model/model_io.h>
#include <falcon/common.h>

using namespace std;

class LinearParameterServer : public ParameterServer {
 public:
  // party
  Party party;
 public:
  /**
   * default constructor
   */
  LinearParameterServer() = default;

  /**
   * constructor
   *
   * @param m_party: the party object
   * @param ps_network_config_pb_str: the network config between ps and worker
   */
  LinearParameterServer(const Party& m_party,const std::string& ps_network_config_pb_str);

  /**
   * copy constructor
   *
   * @param LinearParameterServer
   */
  LinearParameterServer(const LinearParameterServer& obj);

  /**
   * destructor
   */
  ~LinearParameterServer();

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
   * distributed prediction, partition data, collect result, deserialize
   *
   * @param cur_test_data_indexes: vector of index
   * @param predicted_labels: return value, array of labels
   */
  void distributed_predict(
      const std::vector<int>& cur_test_data_indexes,
      EncodedNumber* predicted_labels) override;

 public:
  /**
   * send encrypted weights to workers
   */
  void broadcast_encrypted_weights(const LinearModel& linear_model);

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
   * @param weight_size: the weight size of the local model
   * @param weight_phe_precision: the precision of the local model
   * @param updated_weights: returned aggregated weights
   */
  void update_encrypted_weights(
      const std::vector< string >& encoded_messages,
      int weight_size,
      int weight_phe_precision,
      EncodedNumber* updated_weights) const;
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_PS_H_
