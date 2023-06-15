/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by root on 3/29/22.
//

#include <falcon/inference/interpretability/lime/lime_ps.h>
#include <falcon/utils/logger/logger.h>

LimeParameterServer::LimeParameterServer(const LimeParameterServer &obj)
    : party(obj.party) {}

LimeParameterServer::LimeParameterServer(
    const Party &m_party, const std::string &ps_network_config_pb_str)
    : ParameterServer(ps_network_config_pb_str), party(m_party) {
  log_info("[LimeParameterServer::LimeParameterServer]: constructor. Test "
           "party's content.");
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  log_info("[LimeParameterServer::LimeParameterServer]: okay.");
  djcs_t_free_public_key(phe_pub_key);
}

LimeParameterServer::~LimeParameterServer() = default;

std::vector<int>
LimeParameterServer::partition_examples(std::vector<int> batch_indexes) {

  int mini_batch_size =
      int(batch_indexes.size() / this->worker_channels.size());

  log_info("ps worker size = " + std::to_string(this->worker_channels.size()));
  log_info("mini batch size = " + std::to_string(mini_batch_size));

  std::vector<int> message_sizes;
  // deterministic partition given the batch indexes
  int index = 0;
  for (int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++) {

    // generate mini-batch for this worker
    std::vector<int>::const_iterator first1 = batch_indexes.begin() + index;
    std::vector<int>::const_iterator last1 =
        batch_indexes.begin() + index + mini_batch_size;

    if (wk_index == this->worker_channels.size() - 1) {
      last1 = batch_indexes.end();
    }
    std::vector<int> mini_batch_indexes(first1, last1);

    // serialize mini_batch_indexes to str
    std::string mini_batch_indexes_str;
    serialize_int_array(mini_batch_indexes, mini_batch_indexes_str);

    // record size of mini_batch_indexes, used in deserialization process
    message_sizes.push_back((int)mini_batch_indexes.size());

    // send to worker
    this->send_long_message_to_worker(wk_index, mini_batch_indexes_str);
    // update index
    index += mini_batch_size;
  }

  for (int i = 0; i < message_sizes.size(); i++) {
    log_info("message_sizes[" + std::to_string(i) +
             "] = " + std::to_string(message_sizes[i]));
  }
  log_info("Broadcast client's batch requests to other workers");

  return message_sizes;
}

std::vector<string> LimeParameterServer::wait_worker_complete() {
  std::vector<string> encoded_messages;
  // wait until all received str has been stored to encrypted_gradient_strs
  while (this->worker_channels.size() != encoded_messages.size()) {
    for (int wk_index = 0; wk_index < this->worker_channels.size();
         wk_index++) {
      std::string message;
      this->recv_long_message_from_worker(wk_index, message);
      encoded_messages.push_back(message);
    }
  }
  return encoded_messages;
}

void LimeParameterServer::distributed_train() {}

void LimeParameterServer::distributed_eval(
    falcon::DatasetType eval_type, const std::string &report_save_path) {}

void LimeParameterServer::distributed_predict(
    const std::vector<int> &cur_test_data_indexes,
    EncodedNumber *predicted_labels) {}

void LimeParameterServer::save_model(const std::string &model_save_file) {}