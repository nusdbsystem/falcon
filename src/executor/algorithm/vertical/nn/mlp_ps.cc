//
// Created by root on 5/26/22.
//

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/nn/mlp_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/nn_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>
#include <chrono>
#include <stdexcept>
#include <random>

MlpParameterServer::MlpParameterServer(
    const MlpParameterServer &obj) : party(obj.party) {}

MlpParameterServer::MlpParameterServer(
    const Party &m_party, const std::string &ps_network_config_pb_str) :
    ParameterServer(ps_network_config_pb_str), party(m_party) {
      log_info("[MlpParameterServer::MlpParameterServer]: constructor.");
}

void MlpParameterServer::broadcast_train_test_data(const std::vector<std::vector<double>> &training_data,
                                                   const std::vector<std::vector<double>> &testing_data,
                                                   const std::vector<double> &training_labels,
                                                   const std::vector<double> &testing_labels) {
  // every ps sends the training data and testing data to workers
  std::string training_data_str, testing_data_str, training_labels_str, testing_labels_str;
  serialize_double_matrix(training_data, training_data_str);
  serialize_double_matrix(testing_data, testing_data_str);
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, training_data_str);
    this->send_long_message_to_worker(wk_index, testing_data_str);
  }
  // only active ps sends the training labels and testing labels to workers
  if (party.party_type == falcon::ACTIVE_PARTY) {
    serialize_double_array(training_labels, training_labels_str);
    serialize_double_array(testing_labels, testing_labels_str);
    for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
      this->send_long_message_to_worker(wk_index, training_labels_str);
      this->send_long_message_to_worker(wk_index, testing_labels_str);
    }
  }
}

void MlpParameterServer::broadcast_phe_keys() {
  std::string phe_keys_str = party.export_phe_key_string();
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, phe_keys_str);
  }
}

void MlpParameterServer::broadcast_encrypted_weights(const MlpModel &mlp_model) {
  std::string mlp_model_str;
  serialize_mlp_model(mlp_model, mlp_model_str);

  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){
    this->send_long_message_to_worker(wk_index, mlp_model_str);
  }
}

std::vector<int> MlpParameterServer::partition_examples(std::vector<int> batch_indexes) {
  int mini_batch_size = int(batch_indexes.size()/this->worker_channels.size());
  log_info("ps worker size = " + std::to_string(this->worker_channels.size()));
  log_info("mini batch size = " + std::to_string(mini_batch_size));
  std::vector<int> message_sizes;
  // deterministic partition given the batch indexes
  int index = 0;
  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){
    // generate mini-batch for this worker
    std::vector<int>::const_iterator first1 = batch_indexes.begin() + index;
    std::vector<int>::const_iterator last1  = batch_indexes.begin() + index + mini_batch_size;

    if (wk_index == this->worker_channels.size() - 1) {
      last1  = batch_indexes.end();
    }
    std::vector<int> mini_batch_indexes(first1, last1);
    // serialize mini_batch_indexes to str
    std::string mini_batch_indexes_str;
    serialize_int_array(mini_batch_indexes, mini_batch_indexes_str);
    // record size of mini_batch_indexes, used in deserialization process
    message_sizes.push_back((int) mini_batch_indexes.size());
    // send to worker
    this->send_long_message_to_worker(wk_index, mini_batch_indexes_str);
    // update index
    index += mini_batch_size;
  }

  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) + "] = " + std::to_string(message_sizes[i]));
  }
  log_info("Broadcast client's batch requests to other workers");

  return message_sizes;
}

std::vector<string> MlpParameterServer::wait_worker_complete() {
  std::vector<string> encoded_messages;
  // wait until all received str has been stored to encrypted_gradient_strs
  while (this->worker_channels.size() != encoded_messages.size()){
    for(int wk_index=0; wk_index< this->worker_channels.size(); wk_index++){
      std::string message;
      this->recv_long_message_from_worker(wk_index, message);
      encoded_messages.push_back(message);
    }
  }
  return encoded_messages;
}

void MlpParameterServer::distributed_predict(const std::vector<int> &cur_test_data_indexes,
                                             EncodedNumber *predicted_labels) {

}

void MlpParameterServer::update_encrypted_weights(const std::vector<string> &encoded_messages,
                                                  int weight_size,
                                                  int weight_phe_precision,
                                                  EncodedNumber *updated_weights) const {

}