//
// Created by root on 3/29/22.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_PS_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_PS_H_

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/model/model_io.h>
#include <falcon/common.h>

class LimeParameterServer : public ParameterServer {
 public:
  // party
  Party party;
 public:
  LimeParameterServer() = default;
  LimeParameterServer(const Party& m_party,const std::string& ps_network_config_pb_str);

  /**
   * copy constructor
   * @param LimeParameterServer
   */
  LimeParameterServer(const LimeParameterServer& obj);

  ~LimeParameterServer();

 public:

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
 * abstract method of distributed_train
 */
  void distributed_train() override;

  /**
   * abstract distributed evaluation
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  void distributed_eval(falcon::DatasetType eval_type,
                        const std::string& report_save_path) override;

  void save_model(const std::string& model_save_file) override;

 public:

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

};

#endif //FALCON_INCLUDE_FALCON_INFERENCE_INTERPRETABILITY_LIME_LIME_PS_H_
