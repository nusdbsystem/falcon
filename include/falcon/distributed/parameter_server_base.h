//
// Created by nai li xing on 04/07/21.
//

#ifndef FALCON_PARAMETER_SERVER_H
#define FALCON_PARAMETER_SERVER_H

#include <falcon/network/Comm.hpp>
#include <falcon/party/party.h>

using namespace std;

class ParameterServer {
 public:
  // boost i/o functionality
  boost::asio::io_service io_service;
  // communication channel between parameter server and follower
  std::vector<shared_ptr<CommParty> > worker_channels;

 public:
  ParameterServer() = default;
  explicit ParameterServer(const std::string &ps_network_config_pb_str);
  ~ParameterServer() = default;

  /**
   * send long message to worker via channel commParty
   * @param message: sent message
   */
  void send_long_message_to_worker(int id, std::string message) const;

  /**
   * receive message from worker via channel comm_party
   * @param message: received message
   */
  void recv_long_message_from_worker(int id, std::string &message) const;

  /**
   * PS broadcast the string to all registered workers
   * @param serialized_str the string to be sent
   */
  void broadcast_string_to_workers(const string &serialized_str) const;

  /**
   * abstract method of distributed_train
   */
  virtual void distributed_train() = 0;

  /**
   * abstract distributed evaluation
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  virtual void distributed_eval(falcon::DatasetType eval_type,
                                const std::string &report_save_path) = 0;

  /**
   * abstract distributed prediction
   *
   * @param cur_test_data_indexes: vector of index
   * @param predicted_labels: return value, array of labels
   */
  virtual void distributed_predict(
      const std::vector<int> &cur_test_data_indexes,
      EncodedNumber *predicted_labels) = 0;

  /**
   * abstract method of saving the trained model
   *
   * @param model_save_file: vector of index
   */
  virtual void save_model(const std::string &model_save_file) = 0;

};

#endif //FALCON_PARAMETER_SERVER_H
