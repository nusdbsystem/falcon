//
// Created by root on 11/17/21.
//

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/linear_model_ps.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
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

using namespace std;

LinearParameterServer::LinearParameterServer(
    const LinearParameterServer &obj) : party(obj.party) {}

LinearParameterServer::LinearParameterServer(
    const Party &m_party, const std::string &ps_network_config_pb_str) :
    ParameterServer(ps_network_config_pb_str), party(m_party) {
  log_info("[LinearParameterServer::LinearParameterServer]: constructor.");
}

LinearParameterServer::~LinearParameterServer() = default;

void LinearParameterServer::broadcast_train_test_data(const std::vector<std::vector<double>> &training_data,
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

void LinearParameterServer::broadcast_phe_keys() {
  std::string phe_keys_str = party.export_phe_key_string();
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {
    this->send_long_message_to_worker(wk_index, phe_keys_str);
  }
}

void LinearParameterServer::broadcast_encrypted_weights(const LinearModel& linear_model){
  std::string weight_str;
  serialize_encoded_number_array(linear_model.local_weights,
      linear_model.weight_size,weight_str);

  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){
    this->send_long_message_to_worker(wk_index, weight_str);
  }
}

std::vector<int> LinearParameterServer::partition_examples(std::vector<int> batch_indexes) {
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

std::vector<string> LinearParameterServer::wait_worker_complete(){
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

void LinearParameterServer::distributed_predict(
    const std::vector<int>& cur_test_data_indexes,
    EncodedNumber* decrypted_labels) {
  log_info("current channel size = " + std::to_string(this->worker_channels.size()));
  // step 1: partition sample ids, every ps partition in the same way
  std::vector<int> message_sizes = this->partition_examples(cur_test_data_indexes);
  log_info("cur_test_data_indexes.size = " + std::to_string(cur_test_data_indexes.size()));
  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) + "] = " + std::to_string(message_sizes[i]));
  }
  // step 2: if active party, wait worker finish execution
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::vector< string > encoded_messages = this->wait_worker_complete();
    int cur_index = 0;
    // deserialize encrypted predicted labels
    for (int i = 0; i < encoded_messages.size(); i++){
      auto partial_predicted_labels = new EncodedNumber[message_sizes[i]];
      deserialize_encoded_number_array(
          partial_predicted_labels,
          message_sizes[i],
          encoded_messages[i]);
      for (int k = 0; k < message_sizes[i]; k++){
        decrypted_labels[cur_index] = partial_predicted_labels[k];
        cur_index += 1;
      }
      delete[] partial_predicted_labels;
    }
  }
}

void LinearParameterServer::update_encrypted_weights(
    const std::vector<string> &encoded_messages,
    int weight_size,
    int weight_phe_precision,
    EncodedNumber *updated_weights) const {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  auto encrypted_aggregated_gradients = new EncodedNumber[weight_size];
  // first, need to initialize and encrypt the gradients
  for (int i = 0; i < weight_size; i++) {
    encrypted_aggregated_gradients[i].set_double(phe_pub_key->n[0], 0.0, PHE_FIXED_POINT_PRECISION);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                       encrypted_aggregated_gradients[i],
                       encrypted_aggregated_gradients[i]);
  }

  log_info("[LinearParameterServer::update_encrypted_weights] decode encrypted messages and add to encrypted gradients");

  // deserialize encrypted message, and add to encrypted
  for (const std::string& message: encoded_messages){
    auto tmp = new EncodedNumber[weight_size];
    deserialize_encoded_number_array(
        tmp,
        weight_size,
        message);
    for (auto i = 0; i < weight_size; i++) {
      djcs_t_aux_ee_add_ext(phe_pub_key,
                            encrypted_aggregated_gradients[i],
                            encrypted_aggregated_gradients[i],
                            tmp[i]);
    }
    delete [] tmp;
  }

  log_info("[LinearParameterServer::update_encrypted_weights] aggregate to updated_weights");

  // on parameter server, does not need to differentiate regularization
  // or not, because the regularized gradients is computed on each party
  // and included into the returned gradients, but may need to tune the
  // hyper-parameter learning_rate and alpha
  // after aggregating the gradients, update the local weights
  // update each local weight j
  for (int j = 0; j < weight_size; j++) {
    // update the j-th weight in local_weight vector
    // need to make sure that the exponents of inner_product
    // and local weights are the same
    djcs_t_aux_ee_add_ext(
        phe_pub_key,
        updated_weights[j],
        updated_weights[j],
        encrypted_aggregated_gradients[j]);
  }

  djcs_t_free_public_key(phe_pub_key);
  delete [] encrypted_aggregated_gradients;
}