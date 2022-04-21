//
// Created by root on 11/13/21.
//

#include <falcon/common.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>

#include <cmath>
#include <glog/logging.h>
#include <stack>
#include <iostream>
#include <iomanip>

LinearModel::LinearModel() {}

LinearModel::LinearModel(int m_weight_size) {
  weight_size = m_weight_size;
  local_weights = new EncodedNumber[weight_size];
}

LinearModel::~LinearModel() {
  delete [] local_weights;
}

LinearModel::LinearModel(const LinearModel &linear_model) {
  weight_size = linear_model.weight_size;
  local_weights = new EncodedNumber[weight_size];
  for (int i = 0; i < weight_size; i++) {
    local_weights[i] = linear_model.local_weights[i];
  }
  party_weight_sizes = linear_model.party_weight_sizes;
}

LinearModel &LinearModel::operator=(const LinearModel &linear_model) {
  weight_size = linear_model.weight_size;
  local_weights = new EncodedNumber[weight_size];
  for (int i = 0; i < weight_size; i++) {
    local_weights[i] = linear_model.local_weights[i];
  }
  party_weight_sizes = linear_model.party_weight_sizes;
}

void LinearModel::compute_batch_phe_aggregation(const Party &party,
                                                int cur_batch_size,
                                                EncodedNumber **encoded_batch_samples,
                                                int precision,
                                                EncodedNumber *encrypted_batch_aggregation) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]: getter_phe_pub_key success");

  // each party compute local homomorphic aggregation
  auto* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
  djcs_t_aux_vec_mat_ep_mult(phe_pub_key, party.phe_random,
                         local_batch_phe_aggregation, local_weights,
                         encoded_batch_samples, cur_batch_size, weight_size);

  log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]: djcs_t_aux_vec_mat_ep_mult success");

  // every party sends the local aggregation to the active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self local aggregation results
    for (int i = 0; i < cur_batch_size; i++) {
      // each element (i) in local_batch_phe_aggregation is [W].Xi,
      // Xi is a part of feature vector in example i
      encrypted_batch_aggregation[i] = local_batch_phe_aggregation[i];
    }

    // 1. active party receive serialized string from other parties
    // deserialize and sum to batch_phe_aggregation
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        auto* recv_batch_phe_aggregation =
            new EncodedNumber[cur_batch_size];
        std::string recv_local_aggregation_str;
        try {
          party.recv_long_message(id, recv_local_aggregation_str);
        } catch (const boost::system::system_error& ex)
        {
          std::cout << "Failed to receive. " << ex.what() << std::endl;
        } catch (const std::string& exs) {
          log_info("String error. " + exs);
        } catch (...) {
          log_info("Other errors. ");
        }
        log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]: "
                 "active party recv_long_message from " + std::to_string(id) + " success");
        deserialize_encoded_number_array(recv_batch_phe_aggregation,
                                         cur_batch_size,
                                         recv_local_aggregation_str);
        // homomorphic addition of the received aggregations
        // After addition, each element (i) in batch_phe_aggregation is
        // [W1].Xi1+[W2].Xi2+...+[Wn].Xin Xin is example i's n-th feature,
        // W1 is first element in weigh vector
        for (int i = 0; i < cur_batch_size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_aggregation[i],
                            encrypted_batch_aggregation[i],
                            recv_batch_phe_aggregation[i]);
        }
        delete[] recv_batch_phe_aggregation;
      }
    }
    log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]: "
             "active party do djcs_t_aux_ee_add success");
    // 2. active party serialize and send the aggregated
    // batch_phe_aggregation to other parties
    std::string global_aggregation_str;
    serialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                   global_aggregation_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, global_aggregation_str);
      }
    }
    log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]: "
             "active party send_long_message success");
  } else {
    // if it's passive party, it will not do the aggregation,
    // 1. it just serializes local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_batch_phe_aggregation, cur_batch_size,
                                   local_aggregation_str);
    try {
      party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);
    } catch (const boost::system::system_error& ex)
    {
      std::cout << "Failed to receive. " << ex.what() << std::endl;
    } catch (const std::string& exs) {
      log_info("String error. " + exs);
    } catch (...) {
      log_info("Other errors. ");
    }
    log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]: "
             "passive party send_long_message success");
    // 2. passive party receive the global batch aggregation
    // from the active party
    std::string recv_global_aggregation_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
    deserialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                     recv_global_aggregation_str);
    log_info("[LogisticRegressionModel.compute_batch_phe_aggregation]:"
             " passive party deserialize_encoded_number_array success");
  }

  djcs_t_free_public_key(phe_pub_key);
  delete[] local_batch_phe_aggregation;
}

void LinearModel::encode_samples(const Party &party,
                                 const std::vector<std::vector<double>> &used_samples,
                                 EncodedNumber **encoded_samples,
                                 int precision) const {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // retrieve batch samples and encode (notice to use cur_batch_size
  // instead of default batch size to avoid unexpected batch)
  int cur_sample_size = used_samples.size();

  for (int i = 0; i < cur_sample_size; i++) {
    for (int j = 0; j < weight_size; j++) {
      encoded_samples[i][j].set_double(
          phe_pub_key->n[0],
          used_samples[i][j],
          precision);
    }
  }

  djcs_t_free_public_key(phe_pub_key);
}

void LinearModel::sync_up_weight_sizes(const Party &party) {
  std::vector<int> sync_arr = party.sync_up_int_arr(weight_size);
  party_weight_sizes = sync_arr;
}

std::vector<double> LinearModel::display_weights(Party party) {
  std::vector<double> local_model_weights;
  log_info("display local weights");
  for (int i = 0; i < party.party_num; i++) {
    log_info("party " + std::to_string(i) + "'s weight size = " + std::to_string(party_weight_sizes[i]));
  }
  for (int i = 0; i < party.party_num; i++) {
    log_info("decrypt party " + std::to_string(i) + "'s weights");
    int party_i_weight_size = party_weight_sizes[i];
    auto* party_i_local_weights = new EncodedNumber[party_i_weight_size];
    auto* party_i_decrypted_weights = new EncodedNumber[party_i_weight_size];
    // broadcast local weights and call decrypt
    if (i == party.party_id) {
      log_info("I am party " + std::to_string(i) + ", i am packaging weights");
      for (int j = 0; j < party_i_weight_size; j++) {
        party_i_local_weights[j] = local_weights[j];
      }
    }
    party.broadcast_encoded_number_array(party_i_local_weights, party_i_weight_size, i);
    collaborative_decrypt(party, party_i_local_weights, party_i_decrypted_weights,
                          party_i_weight_size, i);
    if (i == party.party_id) {
      for (int j = 0; j < party_i_weight_size; j++) {
        double weight;
        party_i_decrypted_weights[j].decode(weight);
        std::cout << "local weight[" << j << "] = " << std::setprecision(17)
                  << weight << std::endl;
        LOG(INFO) << "local weight[" << j << "] = " << std::setprecision(17)
                  << weight;
        local_model_weights.push_back(weight);
      }
    }
    delete [] party_i_local_weights;
    delete [] party_i_decrypted_weights;
  }
  return local_model_weights;
//  if (party.party_type == falcon::ACTIVE_PARTY) {
//    auto* decrypted_local_weights = new EncodedNumber[weight_size];
//    for (int i = 0; i < party.party_num; i++) {
//      if (i != party.party_id) {
//        std::string weight_str;
//        serialize_encoded_number_array(local_weights,
//                                       weight_size, weight_str);
//        std::string size_str = std::to_string(weight_size);
//        party.send_long_message(i, size_str);
//        party.send_long_message(i, weight_str);
//      }
//    }
//    party.collaborative_decrypt(local_weights,
//                                decrypted_local_weights,
//                                weight_size,
//                                ACTIVE_PARTY_ID);
//    for (int i = 0; i < weight_size; i++) {
//      double weight;
//      decrypted_local_weights[i].decode(weight);
//      std::cout << "local weight[" << i << "] = " << std::setprecision(17)
//                << weight << std::endl;
//      LOG(INFO) << "local weight[" << i << "] = " << std::setprecision(17)
//                << weight;
//    }
//    delete[] decrypted_local_weights;
//  } else {
//    std::string recv_weight_str, recv_size_str;
//    party.recv_long_message(ACTIVE_PARTY_ID, recv_size_str);
//    party.recv_long_message(ACTIVE_PARTY_ID, recv_weight_str);
//    int weight_num = std::stoi(recv_size_str);
//    auto* received_party_weights = new EncodedNumber[weight_num];
//    auto* decrypted_party_weights = new EncodedNumber[weight_num];
//    deserialize_encoded_number_array(received_party_weights, weight_num,
//                                     recv_weight_str);
//    party.collaborative_decrypt(received_party_weights, decrypted_party_weights,
//                                weight_num, ACTIVE_PARTY_ID);
//    delete[] received_party_weights;
//    delete[] decrypted_party_weights;
//  }
}