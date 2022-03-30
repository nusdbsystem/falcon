//
// Created by nai li xing on 08/12/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_DT_PS_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_DT_PS_H_


#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/tree//tree_builder.h>

#include <utility>


class DTParameterServer: public ParameterServer{
 private:
  // tree model builder
  DecisionTreeBuilder alg_builder;
  // party
  Party party;

 public:
  /**
  * constructor
  * @param m_alg_builder: the module builder
  * @param m_alg_builder: the module builder
  * @param m_alg_builder: the module builder
  */
  DTParameterServer(const DecisionTreeBuilder& m_alg_builder,
                    const Party& m_party,
                    const std::string& ps_network_config_pb_str):
                    ParameterServer(ps_network_config_pb_str),
                    alg_builder(m_alg_builder),party(m_party){};

  /**
  * copy constructor
  * @param obj
  */
  DTParameterServer(const DTParameterServer& obj);

  ~DTParameterServer();

 public:

  /**
  * partition data by features, and send partitioned data to workers
  *
  * @param training_data: split training dataset
  * @param testing_data: split testing dataset
  * @param training_labels: split training labels
  * @param testing_labels: split testing labels
  */

  void broadcast_train_test_data(
      const std::vector<std::vector<double>> &training_data,
      const std::vector<std::vector<double>> &testing_data,
      const std::vector<double> &training_labels,
      const std::vector<double> &testing_labels);


  /**
   * send the phe keys to workers
   */
  void broadcast_phe_keys();

  /**
   * distributed training process, partition data, collect result, update weights
   */
  void distributed_train() override;

  /**
   * distributed lime train tree builder
   *
   * @param use_encrypted_labels: whether use encrypted labels during training
   * @param encrypted_true_labels: encrypted labels used
   * @param use_sample_weights: whether use encrypted sample weights
   * @param encrypted_sample_weights: encrypted sample weights
   */
  void distributed_lime_train(bool use_encrypted_labels,
                              EncodedNumber* encrypted_true_labels,
                              bool use_sample_weights,
                              EncodedNumber* encrypted_sample_weights);

  /**
   * Iterately build the tree
   */
  void build_tree();

  /**
   * distributed evaluation, partition data, collect result, compute evaluation matrix
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  void distributed_eval(
      falcon::DatasetType eval_type,
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
   * partition examples according to number of workers
   *
   * @param encoded_msg: encoded message received from worker
   */
  std::vector<double> retrieve_global_best_split(const std::vector<string>& encoded_msg);

  /**
   * wait until receive all messages returned from worker
   *
   */
  std::vector<string> wait_worker_complete();


  /**
 * partition examples according to number of workers
 *
 * @param batch_indexes: batch's sample index
 */
  std::vector<int> partition_examples(std::vector<int> batch_indexes);

};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_TREE_DT_PS_H_
