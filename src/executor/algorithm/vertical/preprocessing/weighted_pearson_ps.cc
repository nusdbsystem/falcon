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
// Created by root on 20/12/22.
//

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson_ps.h"

WeightedPearsonPS::WeightedPearsonPS(const WeightedPearsonPS &obj) : party(obj.party) {}

WeightedPearsonPS::WeightedPearsonPS(
    const Party &m_party, const string &ps_network_config_pb_str) :
    ParameterServer(ps_network_config_pb_str), party(m_party) {
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  log_info("[WeightedPearsonPS::initialized]: okay.");
  djcs_t_free_public_key(phe_pub_key);
}

std::vector<int> WeightedPearsonPS::partition_examples(std::vector<int> nums){

  int mini_batch_size = int(nums.size()/this->worker_channels.size());

  log_info("[WeightedPearsonPS.partition_examples]: ps worker size = " + std::to_string(this->worker_channels.size()));
  log_info("[WeightedPearsonPS.partition_examples]: mini_batch_size = " + std::to_string(mini_batch_size));
  log_info("[WeightedPearsonPS.partition_examples]: nums = " + std::to_string(nums.size()));

  std::vector<int> message_sizes;
  // deterministic partition given the batch indexes
  int index = 0;
  for(int wk_index = 0; wk_index < this->worker_channels.size(); wk_index++){
    // generate mini-batch for this worker
    std::vector<int>::const_iterator first1 = nums.begin() + index;
    std::vector<int>::const_iterator last1  = nums.begin() + index + mini_batch_size;

    if (wk_index == this->worker_channels.size() - 1){
      last1  = nums.end();
    }
    std::vector<int> mini_batch_indexes(first1, last1);

    // serialize mini_batch_indexes to str
    std::string mini_batch_indexes_str;
    serialize_int_array(mini_batch_indexes, mini_batch_indexes_str);

    // record size of mini_batch_indexes, used in deserialization process
    message_sizes.push_back((int) mini_batch_indexes.size());

    log_info("[PS.partition_examples]: ps send batch_size " +
    std::to_string(mini_batch_indexes.size()) +
    " to worker " + std::to_string(wk_index+1) +
    ", first last index are [" + to_string(mini_batch_indexes[0]) +
    ", " +to_string(mini_batch_indexes.back()) + "]");

    // send to worker
    this->send_long_message_to_worker(wk_index, mini_batch_indexes_str);
    // update index
    index += mini_batch_size;
  }
  return message_sizes;
}

void WeightedPearsonPS::distributed_train() {

}
void WeightedPearsonPS::distributed_eval(falcon::DatasetType eval_type, const string &report_save_path) {

}
void WeightedPearsonPS::save_model(const string &model_save_file) {

}
void WeightedPearsonPS::distributed_predict(const vector<int> &cur_test_data_indexes, EncodedNumber *predicted_labels) {

}
