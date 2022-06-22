//
// Created by root on 5/26/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_PS_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_PS_H_

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/nn/mlp.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/model/model_io.h>
#include <falcon/common.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>

class MlpParameterServer : public ParameterServer {
 public:
  // party
  Party party;
  // mlp model builder
  MlpBuilder mlp_builder;

 public:
  /**
   * default constructor
   */
  MlpParameterServer() = default;

  /**
   * constructor
   *
   * @param m_mlp_builder: the mlp model builder
   * @param m_party: the participating party
   * @param ps_network_config_pb_str: the network config between ps and workers
   */
  MlpParameterServer(const MlpBuilder& m_mlp_builder,
                     const Party& m_party,
                     const std::string& ps_network_config_pb_str);

  /**
   * constructor
   *
   * @param m_party: the party object
   * @param ps_network_config_pb_str: the network config between ps and worker
   */
  MlpParameterServer(const Party& m_party,const std::string& ps_network_config_pb_str);

  /**
   * copy constructor
   *
   * @param MlpParameterServer
   */
  MlpParameterServer(const MlpParameterServer& obj);

  /**
    * destructor
    */
  ~MlpParameterServer() = default;

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

 public:
  /**
   * send encrypted weights to workers
   */
  void broadcast_encrypted_weights(const MlpModel& mlp_model);

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
   * @param agg_mlp_model: returned updated mlp_model
   */
  void update_encrypted_weights(
      const std::vector< string >& encoded_messages,
      MlpModel& agg_mlp_model);

  /**
   * distributed training process, partition data, collect result, update weights
   */
  void distributed_train() override;

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
   * distributed evaluation, partition data, collect result, compute evaluation matrix
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  void distributed_eval(falcon::DatasetType eval_type,
                        const std::string& report_save_path) override;

  /**
   * save the trained model
   *
   * @param model_save_file: vector of index
   */
  void save_model(const std::string& model_save_file) override;
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_PS_H_
