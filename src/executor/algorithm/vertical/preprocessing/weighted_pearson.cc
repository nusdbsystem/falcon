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
// Created by naili on 21/8/22.
//

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson.h"
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/utils/pb_converter/network_converter.h>
#include <future>
#include <cmath>
#include <falcon/utils/io_util.h>
#include <omp.h>

void convert_cipher_to_negative(
    djcs_t_public_key *phe_pub_key,
    const EncodedNumber &cipher_value,
    EncodedNumber &result) {
  EncodedNumber neg_one_int;
  neg_one_int.set_integer(phe_pub_key->n[0], -1);
  djcs_t_aux_ep_mul(phe_pub_key, result, cipher_value, neg_one_int);
}

std::vector<int> sync_global_feature_number(const Party &party) {

  std::vector<int> feature_num_array;

  // 0. active party gather all parties features.
  if (party.party_id == falcon::ACTIVE_PARTY) {
    // receive and deserialize_int_array
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_feature_num_str;
        party.recv_long_message(id, recv_feature_num_str);
        std::vector<int> recv_feature_num;
        deserialize_int_array(recv_feature_num, recv_feature_num_str);
        // aggregate each parties' feature number
        feature_num_array.push_back(recv_feature_num[0]);
      } else {
        feature_num_array.push_back(party.getter_feature_num());
      }
    }

    // serialize and send to other parties
    std::string total_feature_num_str;
    serialize_int_array(feature_num_array, total_feature_num_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, total_feature_num_str);
      }
    }
    return feature_num_array;
  }
    // passive perform following
  else {
    // 1.send to active party
    std::vector<int> local_feature_num;
    local_feature_num.push_back(party.getter_feature_num());
    std::string local_feature_num_str;
    serialize_int_array(local_feature_num, local_feature_num_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_feature_num_str);

    // receive total_feature_num from the request party
    std::string received_local_feature_num_str;
    party.recv_long_message(ACTIVE_PARTY_ID, received_local_feature_num_str);

    deserialize_int_array(feature_num_array, received_local_feature_num_str);
    return feature_num_array;
  }
}

std::vector<int> wpcc_feature_selection(Party party,
                                        int num_explained_features,
                                        const std::string& output_path_prefix,
                                        const vector<std::vector<double>> &train_data,
                                        EncodedNumber *predictions,
                                        const vector<double> &sss_sample_weights,
                                        const string &ps_network_str,
                                        int is_distributed,
                                        int distributed_role,
                                        int worker_id) {

  int num_instance = train_data.size();

  // 1. init local explanation result, each party will hold a share for each feature.
  std::vector<int> selected_feat_idx;

  // 2. get datasets
  std::vector<std::vector<double>> used_train_data = train_data;
  int weight_size = (int) used_train_data[0].size();
  party.setter_feature_num(weight_size);
  log_info("[wpcc_feature_selection]: number of instance = " + std::to_string(used_train_data.size()));

  // all parties get this info
  std::vector<int> feature_num_array = sync_global_feature_number(party);

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // begin to selected features
  // centralized
  if (is_distributed == 0) {
    // 3.1 get the weights for each feature.
    std::vector<double> feature_wpcc_share_vec;
    std::vector<int> party_id_loop_ups;
    std::vector<int> party_feature_id_look_ups;

    get_local_features_correlations(party, feature_num_array, train_data, predictions,
                                    sss_sample_weights,
                                    feature_wpcc_share_vec,
                                    party_id_loop_ups,
                                    party_feature_id_look_ups);

    log_info("[wpcc_feature_selection]: 1. Get all features importance done ! Begin to jointly find top K");

    // TODO: if without sleep 5 seconds, an exception will exist on the MPC side, need further check
    sleep(5);

    // 3.2 find the top k over all parties.
    selected_feat_idx =
        jointly_get_top_k_features(party,
                                   feature_num_array,
                                   feature_wpcc_share_vec,
                                   party_id_loop_ups,
                                   party_feature_id_look_ups,
                                   num_explained_features);
    log_info("[wpcc_feature_selection]: 2. Return selected feature index !");

    log_info("[COMPARE]: -------------------Begin to compare with plaintext-------------------");

    // for debug and compare with baseline
    std::vector<double> wpcc_decrypted_vec = jointly_get_top_k_features_plaintext(party,
                                                                                  feature_num_array,
                                                                                  feature_wpcc_share_vec,
                                                                                  party_id_loop_ups,
                                                                                  party_feature_id_look_ups,
                                                                                  num_explained_features);
    std::vector<double> wpcc_plain_vec = get_local_features_correlations_plaintext(party, feature_num_array,
                                                                                   train_data, predictions,
                                                                                   sss_sample_weights,
                                                                                   feature_wpcc_share_vec);

    for (int i = 0; i < wpcc_plain_vec.size(); i++) {
      log_info("[CHECKING_RES]: DEBUG. feature index = " + std::to_string(i)
                   + " wpcc_decrypted_vec = " + std::to_string(wpcc_decrypted_vec[i])
                   + " wpcc_plain_vec = " + std::to_string(wpcc_plain_vec[i])
      );
    }
    log_info("[CHECKING_RES]: DEBUG. wpcc_decrypted_vec = "
                 + vec_to_str<double>(wpcc_decrypted_vec)
                 + " wpcc_plain_vec = " + vec_to_str<double>(wpcc_plain_vec));

    std::vector<double> w;
    // activate party calculate mean_squared_error
    if (party.party_type == falcon::ACTIVE_PARTY) {
      double mean_squared_error_double = mean_squared_error(wpcc_decrypted_vec, wpcc_plain_vec, w);
      std::cout << mean_squared_error_double << std::endl;
      LOG(INFO) << mean_squared_error_double;
    }

#ifdef SAVE_BASELINE
    std::vector<std::vector<double>> write_data_plain1, write_data_plain2;
    write_data_plain1.push_back(wpcc_decrypted_vec);
    std::vector<double> selected_feat_idx_double;
    for (int i : selected_feat_idx) {
      selected_feat_idx_double.push_back((double) i);
    }
    write_data_plain2.push_back(selected_feat_idx_double);
    std::string wpcc_file_plain = output_path_prefix + "/wpcc.plain";
    log_info("wpcc_file_plain = " + wpcc_file_plain);
    char delimiter = ',';
    write_dataset_to_file(write_data_plain1, delimiter, wpcc_file_plain);
    write_dataset_to_file(write_data_plain2, delimiter, wpcc_file_plain);
#endif
  }

    // distributed
  else {
    std::vector<int> global_feature_index_vec;
    std::vector<int> global_partyid_look_up_vec;
    std::vector<int> global_party_local_feature_id_look_up_vec;
    int global_feature_index_begin = 0;
    // either ps and worker will flatten the feature_num_array
    for (int party_id = 0; party_id < party.party_num; party_id++) {
      int party_feature_num = feature_num_array[party_id];
      for (int feature_id = 0; feature_id < party_feature_num; feature_id++) {
        global_feature_index_vec.push_back(global_feature_index_begin);
        global_feature_index_begin += 1;
        global_partyid_look_up_vec.push_back(party_id);
        global_party_local_feature_id_look_up_vec.push_back(feature_id);
      }
    }

    // either ps and worker partition features using same logic
    int workers_size = retrieve_worker_size_from_ps_network_configs(ps_network_str);
    std::vector<std::vector<int>> partition_vec = partition_vec_balanced(feature_num_array, workers_size);
    // std::vector<std::vector<int>> partition_vec = partition_vec_evenly(global_feature_index_vec, workers_size);
    log_info("[wpcc_feature_selection] workers_size = " + std::to_string(workers_size));

    // debug info
    for (int i = 0; i < partition_vec.size(); i++) {
      log_info("[wpcc_feature_selection] partition i = " + std::to_string(i) + "'s result: ");
      for (int j = 0; j < partition_vec[i].size(); j++) {
        log_info("[wpcc_feature_selection] j = " + std::to_string(j) + ", partition_vec[i][j] = "
                     + std::to_string(partition_vec[i][j]));
      }
    }

    // number of instances and its partition vector
    std::vector<int> instance_index_vec;
    for (int index = 0; index < num_instance; index++) {
      instance_index_vec.push_back(index);
    }
    std::vector<std::vector<int>> instance_partition_vec = partition_vec_evenly(instance_index_vec, workers_size);
//    // debug info
//    for (int i = 0; i < instance_partition_vec.size(); i++) {
//      log_info("partition number = " + std::to_string(instance_partition_vec.size()));
//      for (int j = 0; j < instance_partition_vec[i].size(); j++) {
//        log_info("instance_partition_vec[" + std::to_string(i) + "][" +
//          std::to_string(j) + "] = " + std::to_string(instance_partition_vec[i][j]));
//      }
//    }

    int total_feature_num = 0;
    for (auto &ele: feature_num_array) {
      total_feature_num += ele;
    }

    if (is_distributed == 1 && distributed_role == falcon::DistPS) {
      log_info("[PS]: Init WPCC Parameter server");
      // 1. calculate [w], [w_sum], <r>, <q2>
      // 2. broadcast them and partition features.
      // 2. compare and select top K features with higher wpcc
      // 3. store the selected features and send to all workers

      // init results and compute them.
      log_info("[PS]: begin to calculate <w_sum>, <e>list, <w>list, <q2>lists ");
      // init <w_sum>
      std::vector<double> sum_sss_weight_share;

      // init <e> list
      std::vector<vector<double>> two_d_e_share_vec;
      // init <w> list
      std::vector<std::vector<double>> two_d_sss_weights_share;
      // init <q2> lists
      std::vector<double> q2_shares;

      auto ps = new WeightedPearsonPS(party, ps_network_str);

      ps_get_wpcc_pre_info(*ps, train_data, instance_partition_vec, predictions, sss_sample_weights,
                           sum_sss_weight_share,
                           two_d_e_share_vec, two_d_sss_weights_share, q2_shares);

      // 2. broadcast them and partition features.
      log_info("[PS]: begin to broadcast <w_sum>, [F]list, <e>list, <w>list, <q2>lists ");
      // 2.2
      std::string sum_sss_weight_share_str;
      serialize_double_array(sum_sss_weight_share, sum_sss_weight_share_str);
      ps->broadcast_string_to_workers(sum_sss_weight_share_str);

      // 2.3
      // 2.4
      std::string two_d_e_share_vec_str;
      serialize_double_matrix(two_d_e_share_vec, two_d_e_share_vec_str);
      ps->broadcast_string_to_workers(two_d_e_share_vec_str);

      // 2.5
      std::string two_d_sss_weights_share_str;
      serialize_double_matrix(two_d_sss_weights_share, two_d_sss_weights_share_str);
      ps->broadcast_string_to_workers(two_d_sss_weights_share_str);

      // 2.6
      std::string q2_shares_str;
      serialize_double_array(q2_shares, q2_shares_str);
      ps->broadcast_string_to_workers(q2_shares_str);

      // 2. receive all parties local wpcc and compare and select top K features with higher wpcc

      // Create a vector with an initial size of 10
      std::vector<double> global_wpcc_vec(total_feature_num);
      global_wpcc_vec.reserve(total_feature_num);
      log_info("[PS]: begin to receive global_wpcc_vec from all parties, reserve global_wpcc_vec size of ="
                   + to_string(global_wpcc_vec.size()));

      for (int wk_index = 0; wk_index < ps->worker_channels.size(); wk_index++) {
        log_info("[PS]: begin to receive wpcc from worker=" + std::to_string(wk_index));
        std::vector<double> worker_local_wpcc;
        std::string serialized_worker_wpcc_str;
        ps->recv_long_message_from_worker(wk_index, serialized_worker_wpcc_str);
        deserialize_double_array(worker_local_wpcc, serialized_worker_wpcc_str);
        log_info("[PS]: receive wpcc from worker = " + vec_to_str<double>(worker_local_wpcc));

        for (int worker_local_f_id = 0; worker_local_f_id < worker_local_wpcc.size(); worker_local_f_id++) {
          int feature_index = partition_vec[wk_index][worker_local_f_id];
          log_info("[PS]: reading value from worker = "
                       + to_string(wk_index)
                       + " feature index = " + to_string(feature_index)
                       + " assiend_value = " + to_string(worker_local_wpcc[worker_local_f_id])
          );

          global_wpcc_vec[feature_index] = worker_local_wpcc[worker_local_f_id];
        }
      }

      log_info("[PS]: After receiving , size of global_wpcc_vec share = " + std::to_string(global_wpcc_vec.size()));

      std::vector<double> wpcc_plain_double;
      secret_shares_to_plain_double(party, wpcc_plain_double, global_wpcc_vec, total_feature_num,
                                    ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

      // for debug and check
      for (int i : global_feature_index_vec) {
        log_info("[PS]: verifying received WPCC: global_feature_id ="
                     + std::to_string(i)
                     + " wpcc = " + std::to_string(wpcc_plain_double[i])
                     + " party_id = " + std::to_string(global_partyid_look_up_vec[i])
                     + " party_local_id = " + std::to_string(global_party_local_feature_id_look_up_vec[i])
        );
      }

      log_info("[PS]: After receiving from all parties, begin to calculate feature index with highest wpcc");
      selected_feat_idx =
          jointly_get_top_k_features(party,
                                     feature_num_array,
                                     global_wpcc_vec,
                                     global_partyid_look_up_vec,
                                     global_party_local_feature_id_look_up_vec,
                                     num_explained_features);

      log_info("[PS]: Broadcast selected feature index to each party");
      // 3. store the selected features and send to all workers
      std::string selected_feat_idx_str;
      serialize_int_array(selected_feat_idx, selected_feat_idx_str);
      ps->broadcast_string_to_workers(selected_feat_idx_str);

      // clear the memory
      delete ps;
    }

    if (is_distributed == 1 && distributed_role == falcon::DistWorker) {
      auto worker = new Worker(ps_network_str, worker_id);

      // 0. distributed ps wpcc pre info calculation
//      std::vector<double> sss_sample_weights_share;
//      std::string recv_sss_sample_weights_share_str;
//      worker->recv_long_message_from_ps(recv_sss_sample_weights_share_str);
//      deserialize_double_array(sss_sample_weights_share, recv_sss_sample_weights_share_str);
//      log_info("[worker] sss_sample_weights_share.size = " + std::to_string(sss_sample_weights_share.size()));

      auto *mean_y_cipher = new EncodedNumber[1];
      std::string recv_mean_y_cipher_str;
      worker->recv_long_message_from_ps(recv_mean_y_cipher_str);
      deserialize_encoded_number_array(mean_y_cipher, 1, recv_mean_y_cipher_str);

      std::vector<double> partial_e_share_vec;
      worker_calculate_wpcc_batch_pre_info(party, worker_id - 1, sss_sample_weights,
                                           instance_partition_vec, predictions, mean_y_cipher,
                                           partial_e_share_vec);

      // send partial e_share_vec to ps
      std::string partial_e_share_vec_str;
      serialize_double_array(partial_e_share_vec, partial_e_share_vec_str);
      worker->send_long_message_to_ps(partial_e_share_vec_str);
      log_info("[Worker-" + std::to_string(worker_id - 1) + "]: send wpcc pre info partial_e_share_vec to ps");

      log_info("[Worker-" + std::to_string(worker_id - 1) + "]: responsible for global feature id = "
                   + vec_to_str<int>(partition_vec[worker_id - 1]));

      log_info(
          "[Worker-" + std::to_string(worker_id)
              + "]: begin to receive <w_sum>, [F]list, <e>list, <w>list, <q2>lists from PS");
      // 1. receive all infos

      std::vector<double> sum_sss_weight_share;
      std::vector<vector<double>> two_d_e_share_vec;
      std::vector<std::vector<double>> two_d_sss_weights_share;
      std::vector<double> q2_shares;

      // 2.2
      std::string recv_sum_sss_weight_share_str;
      worker->recv_long_message_from_ps(recv_sum_sss_weight_share_str);
      deserialize_double_array(sum_sss_weight_share, recv_sum_sss_weight_share_str);
      // 2.4
      std::string recv_two_d_e_share_vec_str;
      worker->recv_long_message_from_ps(recv_two_d_e_share_vec_str);
      deserialize_double_matrix(two_d_e_share_vec, recv_two_d_e_share_vec_str);
      // 2.5
      std::string recv_two_d_sss_weights_share_str;
      worker->recv_long_message_from_ps(recv_two_d_sss_weights_share_str);
      deserialize_double_matrix(two_d_sss_weights_share, recv_two_d_sss_weights_share_str);
      // 2.6
      std::string recv_q2_shares_str;
      worker->recv_long_message_from_ps(recv_q2_shares_str);
      deserialize_double_array(q2_shares, recv_q2_shares_str);

      // 2. computes wpcc
      std::vector<double> wpcc_vec;
      log_info("[Worker-" + std::to_string(worker_id - 1) + "]: begin to calculate wpcc_vec for vector = "
                   + vec_to_str<int>(partition_vec[worker_id - 1]));
      worker_calculate_wpcc_per_feature(party, train_data, sss_sample_weights, sum_sss_weight_share,
                                        two_d_e_share_vec, two_d_sss_weights_share, q2_shares,
                                        wpcc_vec,
                                        partition_vec[worker_id - 1], global_partyid_look_up_vec,
                                        global_party_local_feature_id_look_up_vec);
      // 3. send to ps for top K selection
      log_info("[Worker-" + std::to_string(worker_id - 1) + "]: begin to send wpcc_vec to PS");
      std::string wpcc_vec_str;
      serialize_double_array(wpcc_vec, wpcc_vec_str);
      worker->send_long_message_to_ps(wpcc_vec_str);

      // 4. receive final feature indexs and store locally.
      log_info("[Worker-" + std::to_string(worker_id - 1) + "]: wait for PS to finish ");
      std::string recv_selected_feat_idx_str;
      worker->recv_long_message_from_ps(recv_selected_feat_idx_str);

      // clear the memory
      delete[] mean_y_cipher;
      delete worker;
    }
  }

  log_info("Pearson Feature Selection Done");
  return selected_feat_idx;
}

void get_local_features_correlations(const Party &party,
                                     const std::vector<int> &party_feature_nums,
                                     const vector<std::vector<double>> &train_data,
                                     EncodedNumber *predictions,
                                     const vector<double> &sss_sample_weights_share,
                                     std::vector<double> &wpcc_vec,
                                     std::vector<int> &party_id_loop_ups,
                                     std::vector<int> &party_feature_id_look_ups
) {
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // 1. init
  // get batch_true_labels_precision
  int batch_true_labels_precision = std::abs(predictions[0].getter_exponent());
  log_info("[pearson_fl]: batch_true_labels_precision = " + std::to_string(batch_true_labels_precision));

  // get number of instance
  int num_instance = train_data.size();

  // convert <w> to 2D metrics. in form of 1*N
  std::vector<std::vector<double>> two_d_sss_weights_share;
  two_d_sss_weights_share.push_back(sss_sample_weights_share);

  // convert prediction into two dimension array, in from of N*1
  auto **two_d_prediction_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    two_d_prediction_cipher[i] = new EncodedNumber[1];
    two_d_prediction_cipher[i][0] = predictions[i];
  }

  // 2. jointly convert <w> into ciphertext [w],
  auto *sss_weight_cipher = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(party,
                           sss_weight_cipher,
                           sss_sample_weights_share,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[pearson_fl]: 1. jointly convert <w> into ciphertext [w], and saved in active party");

  // 3. all parties calculate sum, and convert it to share
  log_info("step 2, getting the sum of weight, assign exp = " + std::to_string(sss_weight_cipher[0].getter_exponent()));

  int weight_cipher_exp = abs(sss_weight_cipher[0].getter_exponent());
  EncodedNumber sum_sss_weight_cipher_num;
  sum_sss_weight_cipher_num.set_double(phe_pub_key->n[0], 0, weight_cipher_exp);
  djcs_t_aux_encrypt(phe_pub_key, party.phe_random, sum_sss_weight_cipher_num, sum_sss_weight_cipher_num);

  for (int i = 0; i < num_instance; i++) {
//    log_info("Debug: exp of the weight number = " + std::to_string(i) + " is "
//                 + std::to_string(sss_weight_cipher[i].getter_exponent()));
    djcs_t_aux_ee_add(phe_pub_key, sum_sss_weight_cipher_num, sum_sss_weight_cipher_num, sss_weight_cipher[i]);
  }
  // convert to 1d array
  auto *sum_sss_weight_cipher = new EncodedNumber[1];
  sum_sss_weight_cipher[0] = sum_sss_weight_cipher_num;

  // 4. active party convert it to secret shares, and each party hold one share
  std::vector<double> sum_sss_weight_share;
  ciphers_to_secret_shares(party,
                           sum_sss_weight_cipher,
                           sum_sss_weight_share,
                           1,
                           ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  log_info("[pearson_fl]: 2. active party convert sum to secret shares, and each party hold one share");

  // 3. active party calculate <w>*[Prediction] and convert it to secret share
  // init result two dimension cipher of 1*1
  auto **two_d_sss_weight_mul_pred_cipher = new EncodedNumber *[1];
  two_d_sss_weight_mul_pred_cipher[0] = new EncodedNumber[1];
  cipher_shares_mat_mul(party,
                        two_d_sss_weights_share,
                        two_d_prediction_cipher, 1, num_instance, num_instance, 1,
                        two_d_sss_weight_mul_pred_cipher);

  log_info("[pearson_fl]: 3. active party calculate <w>*[Prediction] and convert it to secret share");

  auto *mean_y_cipher = new EncodedNumber[1];
  WeightedMean(party,
               sum_sss_weight_share[0],
               two_d_sss_weight_mul_pred_cipher[0],
               mean_y_cipher,
               ACTIVE_PARTY_ID);

  EncodedNumber mean_y_cipher_neg;
  convert_cipher_to_negative(phe_pub_key, mean_y_cipher[0], mean_y_cipher_neg);
  log_info("[pearson_fl]: 4. each party compute -[mean_y]");

  // 5. calculate the (<w> * {[y] - [mean_y]}) ** 2
  std::vector<double> e_share_vec; // N vector
  auto *e_cipher_vec = new EncodedNumber[num_instance];
  for (int index = 0; index < num_instance; index++) {
    // calculate [y_i] - [mean_y_cipher]
    EncodedNumber y_min_mean_y_cipher;
    djcs_t_aux_ee_add_ext(phe_pub_key, y_min_mean_y_cipher, mean_y_cipher_neg, predictions[index]);

    int y_min_mean_y_cipher_pre = std::abs(y_min_mean_y_cipher.getter_exponent());
//    log_info("[pearson_fl]: 5.2 calculate the [y] - [mean_y], result's precision ="
//                 + std::to_string(y_min_mean_y_cipher_pre));

    // convert [y_i] - [mean_y_cipher] into two dim in form of 1*1
    auto **two_d_mean_y_cipher_neg = new EncodedNumber *[1];
    two_d_mean_y_cipher_neg[0] = new EncodedNumber[1];
    two_d_mean_y_cipher_neg[0][0] = y_min_mean_y_cipher;

    // init result 1*1 metris
    auto **sss_weight_mul_y_min_meany_cipher = new EncodedNumber *[1];
    sss_weight_mul_y_min_meany_cipher[0] = new EncodedNumber[1];

    // pick one element from two_d_sss_weights_share each time and wrapper it as two-d vector
    std::vector<std::vector<double>> two_d_sss_weights_share_element;
    std::vector<double> tmp_vec;
    tmp_vec.push_back(sss_sample_weights_share[index]);
    two_d_sss_weights_share_element.push_back(tmp_vec);

    cipher_shares_mat_mul(party,
                          two_d_sss_weights_share_element,
                          two_d_mean_y_cipher_neg, 1, 1, 1, 1,
                          sss_weight_mul_y_min_meany_cipher);

//    log_info("[pearson_fl]: 5.3 convert [y_i] - [mean_y_cipher] into two dim in form of 1*1 ");

    // copy result to e_cipher_vec
    e_cipher_vec[index] = sss_weight_mul_y_min_meany_cipher[0][0];

//    // get es share
//    std::vector<double> es_share;
//    ciphers_to_secret_shares(party,
//                             sss_weight_mul_y_min_meany_cipher[0],
//                             es_share,
//                             1,
//                             ACTIVE_PARTY_ID,
//                             PHE_FIXED_POINT_PRECISION);
//    e_share_vec.push_back(es_share[0]);

    // clear the memory
    delete[] two_d_mean_y_cipher_neg[0];
    delete[] two_d_mean_y_cipher_neg;
    delete[] sss_weight_mul_y_min_meany_cipher[0];
    delete[] sss_weight_mul_y_min_meany_cipher;
  }

  // decrypt and get es share
  ciphers_to_secret_shares(party,
                           e_cipher_vec,
                           e_share_vec,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);

  log_info("[pearson_fl]: 5. all done ");

  // after getting e, calculate q2, y_vec_min_mean_vec in form of 1*1
  auto **y_vec_min_mean_vec = new EncodedNumber *[1];
  y_vec_min_mean_vec[0] = new EncodedNumber[1];

  // calculate [y_i] - [mean_y_cipher] in form of N*1
  auto **y_vec_min_mean_y_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    EncodedNumber tmp_res;
    djcs_t_aux_ee_add_ext(phe_pub_key, tmp_res, mean_y_cipher_neg, predictions[i]);
    y_vec_min_mean_y_cipher[i] = new EncodedNumber[1];
    y_vec_min_mean_y_cipher[i][0] = tmp_res;
  }

  int y_min_mean_y_cipher_pre = std::abs(y_vec_min_mean_y_cipher[0][0].getter_exponent());
  log_info("[pearson_fl]: 6 calculate [y] - [mean_y], result's precision ="
               + std::to_string(y_min_mean_y_cipher_pre));

  // 6. calculate q2
  // make e_share to two dim,
  std::vector<vector<double>> two_d_e_share_vec;
  two_d_e_share_vec.push_back(e_share_vec);
  cipher_shares_mat_mul(party,
                        two_d_e_share_vec, // 1*N
                        y_vec_min_mean_y_cipher, // N * 1
                        1, num_instance, num_instance, 1,
                        y_vec_min_mean_vec);

  // convert q2 into share
  // in form of 1*N
  std::vector<double> q2_shares;
  ciphers_to_secret_shares(party, y_vec_min_mean_vec[0],
                           q2_shares,
                           1,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[pearson_fl]: 7. convert q2 cipher and convert it to share done");

  // init result featureNum * 1
  auto **party_local_tmp_wf = new EncodedNumber *[party.getter_feature_num()];
  for (int i = 0; i < party.getter_feature_num(); i++) {
    party_local_tmp_wf[i] = new EncodedNumber[1];
  }

  // in form of feature_num * sample*num
  auto **local_encrypted_feature = new EncodedNumber *[party.getter_feature_num()];
  for (int i = 0; i < party.getter_feature_num(); i++) {
    local_encrypted_feature[i] = new EncodedNumber[num_instance];
  }

  // party compute in parallel,
  int local_feature_num = party.getter_feature_num();
  auto **feature_vector_plains = new EncodedNumber *[local_feature_num];
  auto *feature_multiply_w_cipher = new EncodedNumber[local_feature_num];
  for (int feature_id = 0; feature_id < local_feature_num; feature_id++) {
    feature_vector_plains[feature_id] = new EncodedNumber[num_instance];
//    feature_multiply_w_cipher[feature_id] = new EncodedNumber[1];
  }

  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int feature_id = 0; feature_id < party.getter_feature_num(); feature_id++) {
    // 1. each party compute F*[W]
    for (int sample_id = 0; sample_id < num_instance; sample_id++) {
      // assign value
      feature_vector_plains[feature_id][sample_id].set_double(phe_pub_key->n[0],
                                                              train_data[sample_id][feature_id],
                                                              PHE_FIXED_POINT_PRECISION);
    }
    // calculate F*[W], 1*1
    log_info("[pearson_fl] 8.1 feature_id " + std::to_string(feature_id) + "mat mul start");
    djcs_t_aux_inner_product(phe_pub_key,
                             party.phe_random,
                             feature_multiply_w_cipher[feature_id],
                             sss_weight_cipher,
                             feature_vector_plains[feature_id],
                             num_instance);
    log_info("[pearson_fl] 8.1 feature_id " + std::to_string(feature_id) + "mat mul finished");

    party_local_tmp_wf[feature_id][0] = feature_multiply_w_cipher[feature_id];

    // each party also encrypt the feature vector first
    for (int sample_id = 0; sample_id < num_instance; sample_id++) {
      local_encrypted_feature[feature_id][sample_id].set_double(phe_pub_key->n[0],
                                                                train_data[sample_id][feature_id],
                                                                PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key,
                         party.phe_random,
                         local_encrypted_feature[feature_id][sample_id],
                         local_encrypted_feature[feature_id][sample_id]);
    }
  }

  for (int feature_id = 0; feature_id < local_feature_num; feature_id++) {
    delete[] feature_vector_plains[feature_id];
//    delete[] feature_multiply_w_cipher[feature_id];
  }
  delete[] feature_vector_plains;
  delete[] feature_multiply_w_cipher;

  log_info("[pearson_fl]: 8. each party compute F*[W] in parallel done");

#ifdef SAVE_BASELINE
  std::vector<double> debug_vec_p;
  std::vector<double> debug_vec_q1;
  std::vector<double> debug_vec_q2;
  std::vector<double> debug_vec_wpcc;
#endif

  // optimize the local feature correlation calculation
  // o1: all the parties jointly compute from step 9 to 9.4
  // o2: for step 9.5, each party execute locally and use omp parallelism
  // o3: for the rest steps, the parties jointly compute and update
  int global_feature_num = std::accumulate(party_feature_nums.begin(), party_feature_nums.end(), 0);
  log_info("[pearson_fl]: global_feature_num = " + std::to_string(global_feature_num));
  auto *mean_f_cipher_vec = new EncodedNumber[global_feature_num];
  auto *squared_mean_f_cipher_vec = new EncodedNumber[global_feature_num];
  std::vector<double> p_shares_vec; // size = global_feature_num
  auto *f_vec_min_mean_vec_share_vec = new EncodedNumber[global_feature_num];
  std::vector<int> start_global_idx_vec;
  start_global_idx_vec.push_back(0);
  int start_pos = 0;
  for (int party_id = 1; party_id < party.party_num; party_id++) {
    start_pos += party_feature_nums[party_id - 1];
    start_global_idx_vec.push_back(start_pos);
  }

  // 1. calculate local feature mean
  for (int party_id = 0; party_id < party.party_num; party_id++) {
    int party_feature_num = party_feature_nums[party_id];
    int start_global_idx = start_global_idx_vec[party_id];
    log_info("[pearson_fl]: party_id = " + std::to_string(party_id) + ", start_global_idx = "
                 + std::to_string(start_global_idx));
    for (int feature_id = 0; feature_id < party_feature_num; feature_id++) {
      // only the current party have this value. other parties have empty, 1*1
      auto *feature_multiply_w_cipher = new EncodedNumber[1];
      if (party.party_id == party_id) {
        log_info("[pearson_fl]: DEBUG. party_id compute local feature = " + std::to_string(party_id));
        feature_multiply_w_cipher[0] = party_local_tmp_wf[feature_id][0];
      }

      // all party jointly compute the get [mean_f], <mean_f>, [-mean_f] and (mean_f)**2
      // get [mean_f], <mean_f>, [-mean_f]
      auto *mean_f_cipher = new EncodedNumber[1];
      std::vector<double> mean_f_share = WeightedMean(party,
                                                      sum_sss_weight_share[0],
                                                      feature_multiply_w_cipher,
                                                      mean_f_cipher,
                                                      party_id);
      EncodedNumber mean_f_cipher_neg;
      convert_cipher_to_negative(phe_pub_key, mean_f_cipher[0], mean_f_cipher_neg);

      // get (mean_f)**2
      auto *squared_mean_f_cipher = new EncodedNumber[1];
      ciphers_ele_wise_multi(party, squared_mean_f_cipher, mean_f_cipher, mean_f_cipher, 1, party_id);

      mean_f_cipher_vec[start_global_idx + feature_id] = mean_f_cipher[0];
      squared_mean_f_cipher_vec[start_global_idx + feature_id] = squared_mean_f_cipher[0];
      log_info("[pearson_fl]: 9. all parties compute mean_f and (mean_f)**2");

      // only the current party calculate [F] - [mean_F], other don;t have value of f_vec_min_mean_f_cipher
      // N*1
      auto **f_vec_min_mean_f_cipher = new EncodedNumber *[num_instance];
      for (int sample_id = 0; sample_id < num_instance; sample_id++) {
        f_vec_min_mean_f_cipher[sample_id] = new EncodedNumber[1];
      }

      if (party.party_id == party_id) {
        EncodedNumber f_min_mean_f_cipher;
        for (int sample_id = 0; sample_id < num_instance; sample_id++) {
          djcs_t_aux_ee_add(phe_pub_key,
                            f_min_mean_f_cipher,
                            mean_f_cipher_neg,
                            local_encrypted_feature[feature_id][sample_id]);
          f_vec_min_mean_f_cipher[sample_id][0] = f_min_mean_f_cipher;
        }
      }
      log_info("[pearson_fl]: 9.1. calculate [f_i] - [mean_f_cipher],");

      // the current party send it to all other parties for future computation
      broadcast_encoded_number_matrix(party, f_vec_min_mean_f_cipher, num_instance, 1, party_id);
      log_info("[pearson_fl]: 9.2. party id " + std::to_string(party_id) + " send [F]-[MeanF] to others");

      // all parties jointly calculate sum(<w_i> * ([f_i] - [mean_F])([y_i] - [mean_Y])), 1*1
      auto **f_vec_min_mean_vec_share = new EncodedNumber *[1];
      f_vec_min_mean_vec_share[0] = new EncodedNumber[1];
      cipher_shares_mat_mul(party,
                            two_d_e_share_vec, // 1*N
                            f_vec_min_mean_f_cipher, // N*1
                            1, num_instance, num_instance, 1,
                            f_vec_min_mean_vec_share);
      log_info("[pearson_fl]: 9.3. all parties jointly calculate sum(<w_i> * ([f_i] - [mean_F])([y_i] - [mean_Y])),");

      f_vec_min_mean_vec_share_vec[start_global_idx + feature_id] = f_vec_min_mean_vec_share[0][0];

      delete[] feature_multiply_w_cipher;
      delete[] mean_f_cipher;
      delete[] squared_mean_f_cipher;
      for (int i = 0; i < num_instance; i++) {
        delete[] f_vec_min_mean_f_cipher[i];
      }
      delete[] f_vec_min_mean_f_cipher;
      delete[] f_vec_min_mean_vec_share[0];
      delete[] f_vec_min_mean_vec_share;
    }
  }

  //  all parties jointly compute p share, 1*1
  ciphers_to_secret_shares(party, f_vec_min_mean_vec_share_vec,
                           p_shares_vec,
                           global_feature_num,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[pearson_fl]: 9.4. all parties jointly calculate p share <p>");
  delete[] f_vec_min_mean_vec_share_vec;

  // calculate q1 = (f-mean_f) ** 2, tmp_vec_cipher: N*1
  // each party calculate in parallel
  auto **tmp_vec_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    tmp_vec_cipher[i] = new EncodedNumber[global_feature_num];
  }
  int start_global_idx = start_global_idx_vec[party.party_id];
  log_info("[pearson_fl]: start_global_idx = " + std::to_string(start_global_idx));

  auto **squared_feature_value_cipher_mat = new EncodedNumber *[num_instance];
  auto **middle_value_mat = new EncodedNumber *[num_instance];
  auto **neg_2f_mat = new EncodedNumber *[num_instance];
  auto **tmp_vec_ele_mat = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    squared_feature_value_cipher_mat[i] = new EncodedNumber[party.getter_feature_num()];
    middle_value_mat[i] = new EncodedNumber[party.getter_feature_num()];
    neg_2f_mat[i] = new EncodedNumber[party.getter_feature_num()];
    tmp_vec_ele_mat[i] = new EncodedNumber[party.getter_feature_num()];
  }

  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int feature_id = 0; feature_id < party.getter_feature_num(); feature_id++) {
    for (int i = 0; i < num_instance; i++) {
      // calculate [f**2]
      // EncodedNumber squared_feature_value_cipher;
      squared_feature_value_cipher_mat[i][feature_id].set_double(phe_pub_key->n[0],
                                                                 train_data[i][feature_id] * train_data[i][feature_id],
                                                                 PHE_FIXED_POINT_PRECISION * PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key,
                         party.phe_random,
                         squared_feature_value_cipher_mat[i][feature_id],
                         squared_feature_value_cipher_mat[i][feature_id]);

      // calculate -2f*[mean_f]`
      // EncodedNumber middle_value;
      // EncodedNumber neg_2f;
      neg_2f_mat[i][feature_id].set_double(phe_pub_key->n[0], -2 * train_data[i][feature_id],
                                           abs(mean_f_cipher_vec[start_global_idx + feature_id].getter_exponent()));
      djcs_t_aux_ep_mul(phe_pub_key,
                        middle_value_mat[i][feature_id],
                        mean_f_cipher_vec[start_global_idx + feature_id],
                        neg_2f_mat[i][feature_id]);

      // calculate [f_j]**2 + -2f_j*[mean_F] + [mean_F]**2
      // EncodedNumber tmp_vec_ele;
      djcs_t_aux_ee_add_ext(phe_pub_key,
                            tmp_vec_ele_mat[i][feature_id],
                            squared_feature_value_cipher_mat[i][feature_id],
                            middle_value_mat[i][feature_id]);
      djcs_t_aux_ee_add_ext(phe_pub_key,
                            tmp_vec_ele_mat[i][feature_id],
                            tmp_vec_ele_mat[i][feature_id],
                            squared_mean_f_cipher_vec[start_global_idx + feature_id]);
      tmp_vec_cipher[i][start_global_idx + feature_id] = tmp_vec_ele_mat[i][feature_id];
    }
  }

  for (int i = 0; i < num_instance; i++) {
    delete[] squared_feature_value_cipher_mat[i];
    delete[] middle_value_mat[i];
    delete[] neg_2f_mat[i];
    delete[] tmp_vec_ele_mat[i];
  }
  delete[] squared_feature_value_cipher_mat;
  delete[] middle_value_mat;
  delete[] neg_2f_mat;
  delete[] tmp_vec_ele_mat;

  log_info("[pearson_fl]: 9.5. for each instance, calculate [f**2] + -2f*[mean_f] + [mean_f**2]");

  // the parties need to sync up the tmp_vec_cipher matrix to make sure they have the same matrix
  // each party send the partial tmp_vec_cipher to active party, the active party aggregate and broadcast
  if (party.party_id == falcon::ACTIVE_PARTY) {
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_tmp_vec_ciphers_str;
        party.recv_long_message(id, recv_tmp_vec_ciphers_str);
        auto **recv_tmp_vec_ciphers = new EncodedNumber *[num_instance];
        for (int i = 0; i < num_instance; i++) {
          recv_tmp_vec_ciphers[i] = new EncodedNumber[global_feature_num];
        }
        deserialize_encoded_number_matrix(recv_tmp_vec_ciphers,
                                          num_instance,
                                          global_feature_num,
                                          recv_tmp_vec_ciphers_str);
        // copy party id's results to tmp_vec_ciphers
        int party_id_start_global_idx = start_global_idx_vec[id];
        int party_id_feature_num = party_feature_nums[id];
        log_info("[pearson_fl]: 9.5.1. party_id = " + std::to_string(id) +
            ", party_id_start_global_idx = " + std::to_string(party_id_start_global_idx) +
            ", party_id_feature_num = " + std::to_string(party_id_feature_num));
        for (int i = 0; i < num_instance; i++) {
          for (int feature_id = 0; feature_id < party_id_feature_num; feature_id++) {
            tmp_vec_cipher[i][party_id_start_global_idx + feature_id] =
                recv_tmp_vec_ciphers[i][party_id_start_global_idx + feature_id];
          }
        }

        for (int i = 0; i < num_instance; i++) {
          delete[] recv_tmp_vec_ciphers[i];
        }
        delete[] recv_tmp_vec_ciphers;
      }
    }
  } else {
    std::string tmp_vec_ciphers_str;
    serialize_encoded_number_matrix(tmp_vec_cipher, num_instance, global_feature_num, tmp_vec_ciphers_str);
    party.send_long_message(ACTIVE_PARTY_ID, tmp_vec_ciphers_str);
  }

  broadcast_encoded_number_matrix(party, tmp_vec_cipher, num_instance, global_feature_num, ACTIVE_PARTY_ID);
  log_info("[pearson_fl]: 9.6. party id " + std::to_string(party.party_id) + " send tmp_vec_cipher to others");


  // all parties jointly calculate q1 cipher and convert it to shares 1*1
  auto **q1_cipher = new EncodedNumber *[1];
  q1_cipher[0] = new EncodedNumber[global_feature_num];
  cipher_shares_mat_mul(party,
                        two_d_sss_weights_share, // 1*N
                        tmp_vec_cipher, // N*global_feature_num
                        1, num_instance, num_instance, global_feature_num,
                        q1_cipher);
  log_info("[pearson_fl]: 9.7. calculate q1 ");

  // convert cipher to shares
  std::vector<double> q1_shares_vec;
  ciphers_to_secret_shares(party, q1_cipher[0],
                           q1_shares_vec,
                           global_feature_num,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);

  std::vector<double> res_wpcc_shares = compute_wpcc(party, p_shares_vec, q1_shares_vec, q2_shares[0]);

  // the parties calculate wpcc shares
  // 1. calculate local feature mean
  int idx = 0;
  for (int party_id = 0; party_id < party.party_num; party_id++) {
    int party_feature_num = party_feature_nums[party_id];
    start_global_idx = start_global_idx_vec[party_id];
    log_info("[pearson_fl]: party_id = " + std::to_string(party_id) + ", start_global_idx = "
                 + std::to_string(start_global_idx));
    for (int feature_id = 0; feature_id < party_feature_num; feature_id++) {
      // all parties jointly calculate  WPCC share, wpcc = <p> / (<q1> * <q2>) for this features
//      double one_wpcc_share = compute_wpcc(party, p_shares_vec[start_global_idx+feature_id],
//                                           q1_shares_vec[start_global_idx+feature_id], q2_shares[0]);

//#ifdef SAVE_BASELINE
//      // debug p, q1, q2
//      std::vector<double> p_plain_double;
//      secret_shares_to_plain_double(party, p_plain_double, p_shares, 1,
//                                    ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
//      std::vector<double> q1_plain_double;
//      secret_shares_to_plain_double(party, q1_plain_double, q1_shares, 1,
//                                    ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
//      std::vector<double> q2_plain_double;
//      secret_shares_to_plain_double(party, q2_plain_double, q2_shares, 1,
//                                    ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
//
//      std::vector<double> wpcc_plain_double;
//      std::vector<double> share_vec_wpcc;
//      share_vec_wpcc.push_back(one_wpcc_share);
//      secret_shares_to_plain_double(party, wpcc_plain_double, share_vec_wpcc, 1,
//                                    ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
//
//      debug_vec_p.push_back(p_plain_double[0]);
//      debug_vec_q1.push_back(q1_plain_double[0]);
//      debug_vec_q2.push_back(q2_plain_double[0]);
//      debug_vec_wpcc.push_back(wpcc_plain_double[0]);
//#endif
      wpcc_vec.push_back(res_wpcc_shares[idx]);
      idx++;
      party_id_loop_ups.push_back(party_id);
      party_feature_id_look_ups.push_back(feature_id);
      log_info("[pearson_fl]: 9.8. calculate WPCC for feature " + std::to_string(feature_id));
    }
  }

  delete[] e_cipher_vec;
  delete[] mean_f_cipher_vec;
  delete[] squared_mean_f_cipher_vec;
  for (int i = 0; i < num_instance; i++) {
    delete[] tmp_vec_cipher[i];
  }
  delete[] tmp_vec_cipher;
  delete[] q1_cipher[0];
  delete[] q1_cipher;

  djcs_t_free_public_key(phe_pub_key);

#ifdef SAVE_BASELINE
  for (int feature_indx = 0; feature_indx < debug_vec_p.size(); feature_indx++) {
    log_info("[wpcc_plaintext]: Debug, system decryption. feature index = " + std::to_string(feature_indx)
                 + " wpcc = " + std::to_string(debug_vec_wpcc[feature_indx])
                 + " p = " + std::to_string(debug_vec_p[feature_indx])
                 + " q1 = " + std::to_string(debug_vec_q1[feature_indx])
                 + " q2 = " + std::to_string(debug_vec_q2[feature_indx])
    );
  }
#endif

  log_info("[pearson_fl]: 10. All done, begin to clear the memory");

  // clean the code
  delete[] sum_sss_weight_cipher;
  for (int i = 0; i < num_instance; i++) {
    delete[] two_d_prediction_cipher[i];
  }
  delete[] two_d_prediction_cipher;
  delete[] sss_weight_cipher;
  for (int i = 0; i < num_instance; i++) {
    delete[] y_vec_min_mean_y_cipher[i];
  }
  delete[] y_vec_min_mean_y_cipher;
  for (int i = 0; i < party.getter_feature_num(); i++) {
    delete[] party_local_tmp_wf[i];
  }
  delete[] party_local_tmp_wf;
  for (int i = 0; i < party.getter_feature_num(); i++) {
    delete[] local_encrypted_feature[i];
  }
  delete[] local_encrypted_feature;
  delete[] two_d_sss_weight_mul_pred_cipher[0];
  delete[] two_d_sss_weight_mul_pred_cipher;
  delete[] mean_y_cipher;
  delete[] y_vec_min_mean_vec[0];
  delete[] y_vec_min_mean_vec;

  log_info("[pearson_fl]: 11. Clear the memory done, return wpcc shares for all features");
}

std::vector<int> jointly_get_top_k_features(const Party &party,
                                            const std::vector<int> &party_feature_nums,
                                            const std::vector<double> &feature_cor_shares,
                                            const std::vector<int> &party_id_loop_ups,
                                            const std::vector<int> &party_feature_id_look_ups,
                                            int num_explained_features) {

  log_info("[weighted_pearson] jointly_get_top_k_features start");

  int total_feature_num = 0;
  for (auto &ele: party_feature_nums) {
    total_feature_num += ele;
  }

  // 1. get the top k features and store their id as share
  // public value record 'K' features needed to be selected.
  std::vector<int> public_values;
  public_values.push_back(num_explained_features);
  public_values.push_back(total_feature_num);

  log_info("[weighted_pearson] ready to call spdz function");

  // set the computation type is select top K
  falcon::SpdzLimeCompType comp_type = falcon::PEARSON_TopK;
  // set the value for reading from threads
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  // start compute
  std::thread spdz_pearson_comp(spdz_lime_computation,
                                party.party_num,
                                party.party_id,
                                party.executor_mpc_ports,
                                party.host_names,
                                public_values.size(),
                                public_values,
                                feature_cor_shares.size(),
                                feature_cor_shares,
                                comp_type,
                                &promise_values);
  // get value feature id in share
  std::vector<double> feature_index_share = future_values.get();
  spdz_pearson_comp.join();

  log_info("[jointly_get_top_k_features] num_explained_features = " + std::to_string(num_explained_features));

  std::vector<double> feature_global_index_double;
  secret_shares_to_plain_double(party, feature_global_index_double,
                                feature_index_share, num_explained_features,
                                ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  // each party only get the local feature index
  std::vector<int> selected_feat_idx;
  for (int i = 0; i < num_explained_features; i++) {
    // decode the plaintext into int, which is the globally selected feature id
    int decoded_feature_global_index_int = static_cast<int>(std::round(feature_global_index_double[i]));

    // convert global feature id into local feature id, and record it.
    log_info("[jointly_get_top_k_features] , feature_global_index_int = "
                 + std::to_string(decoded_feature_global_index_int)
                 + " party_id = " + std::to_string(party_id_loop_ups[decoded_feature_global_index_int])
                 + " party local feature index = "
                 + std::to_string(party_feature_id_look_ups[decoded_feature_global_index_int]));

    int cur_party_id = party_id_loop_ups[decoded_feature_global_index_int];
    if (cur_party_id == party.party_id) {
      int cur_local_feature_id = party_feature_id_look_ups[decoded_feature_global_index_int];
      selected_feat_idx.push_back(cur_local_feature_id);
    }
  }
  return selected_feat_idx;
}

std::vector<double> WeightedMean(const Party &party,
                                 const double &weight_sum_share, // sum of weight share
                                 EncodedNumber *tmp_numerator_cipher, // cipher value
                                 EncodedNumber *weighted_mean_cipher,
                                 int req_party_id
) {

  // each party hold one share

  log_info("[pearson_fl]: DEBUG. req_party_id compute local feature = " + std::to_string(req_party_id));

  std::vector<double> tmp_share;
  ciphers_to_secret_shares(party,
                           tmp_numerator_cipher,
                           tmp_share,
                           1,
                           req_party_id,
                           PHE_FIXED_POINT_PRECISION);

  // party jointly compute [mean_v] = SSS2PHE(<tmp> / <w_sum>)
  // send to MPC
  std::vector<double> private_values;
  private_values.push_back(tmp_share[0]);
  private_values.push_back(weight_sum_share);
  std::vector<int> public_values;
  falcon::SpdzLimeCompType comp_type = falcon::PEARSON_Division;
  std::promise<std::vector<double>> promise_values_mean_y;
  std::future<std::vector<double>> future_values_mean_y = promise_values_mean_y.get_future();
  std::thread spdz_dist_mean_y(spdz_lime_computation,
                               party.party_num,
                               party.party_id,
                               party.executor_mpc_ports,
                               party.host_names,
                               0,
                               public_values, // no public value needed
                               private_values.size(), // 2
                               private_values, // <mean_y>, <sum_w>
                               comp_type,
                               &promise_values_mean_y);
  // each party have a share locally
  std::vector<double> weighted_mean_share_vec = future_values_mean_y.get();
  spdz_dist_mean_y.join();

  // convert mean_y share into mean_y cipher, all party have such cipher
  secret_shares_to_ciphers(party,
                           weighted_mean_cipher,
                           weighted_mean_share_vec,
                           1,
                           req_party_id,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[WeightedMean]: compute weighted_mean_share_vec and communicate with spdz finished");

  return weighted_mean_share_vec;
}

std::vector<double> compute_wpcc(
    const Party &party,
    const std::vector<double> &p_shares_vec,
    const std::vector<double> &q1_shares_vec,
    double q2_shares
) {
  int vec_size = (int) p_shares_vec.size();
  log_info("[compute_wpcc] vec_size = " + std::to_string(vec_size));
  std::vector<int> public_values;
  public_values.push_back(vec_size);

  std::vector<double> private_values_wpcc;
  for (int i = 0; i < vec_size; i++) {
    private_values_wpcc.push_back(p_shares_vec[i]);
  }
  for (int i = 0; i < vec_size; i++) {
    private_values_wpcc.push_back(q1_shares_vec[i]);
  }
  private_values_wpcc.push_back(q2_shares);
  falcon::SpdzLimeCompType comp_type_wpcc = falcon::PEARSON_Div_with_SquareRoot;
  std::promise<std::vector<double>> promise_values_wpcc;
  std::future<std::vector<double>> future_values_wpcc = promise_values_wpcc.get_future();
  std::thread spdz_dist_wpcc(spdz_lime_computation,
                             party.party_num,
                             party.party_id,
                             party.executor_mpc_ports,
                             party.host_names,
                             public_values.size(), // 1
                             public_values, // no public value needed
                             private_values_wpcc.size(), // 2 * vec_size + 1
                             private_values_wpcc, // <mean_y>, <sum_w>
                             comp_type_wpcc,
                             &promise_values_wpcc);
  std::vector<double> wpcc_shares = future_values_wpcc.get();
  spdz_dist_wpcc.join();
  return wpcc_shares;
}

void spdz_lime_computation(int party_num,
                           int party_id,
                           std::vector<int> mpc_port_bases,
                           std::vector<std::string> party_host_names,
                           int public_value_size,
                           const std::vector<int> &public_values,
                           int private_value_size,
                           const std::vector<double> &private_values,
                           falcon::SpdzLimeCompType lime_comp_type,
                           std::promise<std::vector<double>> *res) {
  // Here put the whole setup socket code together, as using a function call
  // would result in a problem when deleting the created sockets
  // setup connections from this party to each spdz party socket
  std::vector<ssl_socket *> mpc_sockets(party_num);
  vector<int> plain_sockets(party_num);
  // ssl_ctx ctx(mpc_player_path, "C" + to_string(party_id));
  ssl_ctx ctx("C" + to_string(party_id));
  // std::cout << "correct init ctx" << std::endl;
  ssl_service io_service;
  octetStream specification;
  log_info("begin connect to spdz parties");
  log_info("party_num = " + std::to_string(party_num));
  for (int i = 0; i < party_num; i++) {
    log_info(
        "[spdz_lime_computation]: falcon client is connecting with MPC server = " + std::to_string(i) + " at port = "
            + std::to_string(mpc_port_bases[i] + i) + " ......");

    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(), mpc_port_bases[i] + i);
    send(plain_sockets[i], (octet *) &party_id, sizeof(int));
    mpc_sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                    "P" + to_string(i), "C" + to_string(party_id), true);
    if (i == 0) {
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << mpc_sockets[i] << ", port_num = "
              << mpc_port_bases[i] + i << ".";
  }
  log_info("Finish setup socket connections to spdz engines.");
  int type = specification.get<int>();
  // todo, 'p' is 112 , but what 112 means?
  switch (type) {
    case 'p': {
      gfp::init_field(specification.get<bigint>());
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:LOG(ERROR) << "Type " << type << " not implemented";
      exit(1);
  }
  log_info("Finish initializing gfp field.");
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;

  // send data to spdz parties
  log_info("party_id = " + std::to_string(party_id));
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(lime_comp_type);
    log_info("lime_comp_type = " + std::to_string(lime_comp_type));
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends public values to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  log_info("active party sending all public value to all mpc, lime_comp_type = " + std::to_string(lime_comp_type));
  google::FlushLogFiles(google::INFO);

  // all the parties send private shares
  log_info("private value size = " + std::to_string(private_value_size));
  for (int i = 0; i < private_value_size; i++) {
    vector<double> x;
    x.push_back(private_values[i]);
    send_private_inputs(x, mpc_sockets, party_num);
  }

  // receive result from spdz parties according to the computation type
  switch (lime_comp_type) {
    case falcon::DIST_WEIGHT: {
      LOG(INFO) << "SPDZ lime computation dist weights computation";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
      res->set_value(return_values);
      break;
    }
    case falcon::PEARSON_Division: {
      LOG(INFO) << "SPDZ calculate mean value <mean_value> / <mean_sum>";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
      res->set_value(return_values);
      break;
    }
    case falcon::PEARSON_Div_with_SquareRoot: {
      log_info("SPDZ calculate mean value <p> /( <q1> * <q2>) ");
      int vec_size = public_values[0];
      log_info("[spdz_lime_computation] vec_size = " + std::to_string(vec_size));
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, vec_size);
      res->set_value(return_values);
      break;
    }
    case falcon::PEARSON_TopK: {
      log_info("falcon::PEARSON_TopK = " + std::to_string(falcon::PEARSON_TopK));
      LOG(INFO) << "SPDZ find top k features with largest wpcc values";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[0]);
      res->set_value(return_values);
      break;
    }
    case falcon::KERNELSHAP_WEIGHT: {
      log_info("falcon::KERNELSHAP_WEIGHT = " + std::to_string(falcon::KERNELSHAP_WEIGHT));
      LOG(INFO) << "SPDZ compute kernelshap weights";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, public_values[0]);
      res->set_value(return_values);
      break;
    }
    default:LOG(INFO) << "SPDZ lime computation type is not found.";
      exit(1);
  }

  for (int i = 0; i < party_num; i++) {
    close_client_socket(plain_sockets[i]);
  }

  // free memory and close mpc_sockets
  for (int i = 0; i < party_num; i++) {
    delete mpc_sockets[i];
    mpc_sockets[i] = nullptr;
  }
}

/***********************************************************/
/***************** logic for PS ***************/
/***********************************************************/

void ps_get_wpcc_pre_info(const WeightedPearsonPS &ps,
                          const vector<std::vector<double>> &train_data,
                          const std::vector<std::vector<int>> &instance_partition_vec,
                          EncodedNumber *predictions,
                          const vector<double> &sss_sample_weights_share,
                          std::vector<double> &sum_sss_weight_share,
                          std::vector<vector<double>> &two_d_e_share_vec,
                          std::vector<std::vector<double>> &two_d_sss_weights_share,
                          std::vector<double> &q2_shares
) {

  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  ps.party.getter_phe_pub_key(phe_pub_key);

  // 1. init
  // get batch_true_labels_precision
  int batch_true_labels_precision = std::abs(predictions[0].getter_exponent());
  log_info("[WPCC_PS]: batch_true_labels_precision = " + std::to_string(batch_true_labels_precision));

  // get number of instance
  int num_instance = train_data.size();

  // convert <w> to 2D metrics. in form of 1*N
  two_d_sss_weights_share.push_back(sss_sample_weights_share);

  // convert prediction into two dimension array, in from of N*1
  auto **two_d_prediction_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    two_d_prediction_cipher[i] = new EncodedNumber[1];
    two_d_prediction_cipher[i][0] = predictions[i];
  }

  // 2. jointly convert <w> into ciphertext [w],
  auto *sss_weight_cipher = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(ps.party,
                           sss_weight_cipher,
                           sss_sample_weights_share,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[WPCC_PS]: 1. jointly convert <w> into ciphertext [w], and saved in active party");

  // 3. all parties calculate sum, and convert it to share
  log_info("[WPCC_PS]: step 2, getting the sum of weight, assign exp = "
               + std::to_string(sss_weight_cipher[0].getter_exponent()));

  int weight_cipher_exp = abs(sss_weight_cipher[0].getter_exponent());
  EncodedNumber sum_sss_weight_cipher_num;
  sum_sss_weight_cipher_num.set_double(phe_pub_key->n[0], 0, weight_cipher_exp);
  djcs_t_aux_encrypt(phe_pub_key, ps.party.phe_random, sum_sss_weight_cipher_num, sum_sss_weight_cipher_num);

  for (int i = 0; i < num_instance; i++) {
//    log_info("[WPCC_PS]: Debug: exp of the weight number = " + std::to_string(i) + " is "
//                 + std::to_string(sss_weight_cipher[i].getter_exponent()));
    djcs_t_aux_ee_add(phe_pub_key, sum_sss_weight_cipher_num, sum_sss_weight_cipher_num, sss_weight_cipher[i]);
  }
  // convert to 1d array
  auto *sum_sss_weight_cipher = new EncodedNumber[1];
  sum_sss_weight_cipher[0] = sum_sss_weight_cipher_num;

  // 4. active party convert it to secret shares, and each party hold one share
  ciphers_to_secret_shares(ps.party,
                           sum_sss_weight_cipher,
                           sum_sss_weight_share,
                           1,
                           ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  log_info("[WPCC_PS]: 2. active party convert sum to secret shares, and each party hold one share");

  // 3. active party calculate <w>*[Prediction] and convert it to secret share
  // init result two dimension cipher of 1*1
  auto **two_d_sss_weight_mul_pred_cipher = new EncodedNumber *[1];
  two_d_sss_weight_mul_pred_cipher[0] = new EncodedNumber[1];
  cipher_shares_mat_mul(ps.party,
                        two_d_sss_weights_share,
                        two_d_prediction_cipher, 1, num_instance, num_instance, 1,
                        two_d_sss_weight_mul_pred_cipher);

  log_info("[WPCC_PS]: 3. active party calculate <w>*[Prediction] and convert it to secret share");

  auto *mean_y_cipher = new EncodedNumber[1];
  WeightedMean(ps.party,
               sum_sss_weight_share[0],
               two_d_sss_weight_mul_pred_cipher[0],
               mean_y_cipher,
               ACTIVE_PARTY_ID);

  EncodedNumber mean_y_cipher_neg;
  convert_cipher_to_negative(phe_pub_key, mean_y_cipher[0], mean_y_cipher_neg);
  log_info("[WPCC_PS]: 4. each party compute -[mean_y]");

  // 5. calculate the (<w> * {[y] - [mean_y]}) ** 2 in parallel on workers
  log_info("[PS]: begin to broadcast <w>, [mean_y_cipher]");
  // 5.1 broadcast sss weights
//  std::string sum_sss_weight_share_str;
//  serialize_double_array(sum_sss_weight_share, sum_sss_weight_share_str);
//  ps.broadcast_string_to_workers(sum_sss_weight_share_str);
//  log_info("[PS] sum_sss_weight_share.size = " + std::to_string(sum_sss_weight_share.size()));


  // 5.2 broadcast mean_y_cipher
  std::string mean_y_cipher_str;
  serialize_encoded_number_array(mean_y_cipher, 1, mean_y_cipher_str);
  ps.broadcast_string_to_workers(mean_y_cipher_str);

  log_info("[PS]: broadcast <w>, [mean_y_cipher] finished");

  // receive all workers' partial_e_share_vec and aggregate
  std::vector<double> e_share_vec; // N vector
  for (int wk_index = 0; wk_index < ps.worker_channels.size(); wk_index++) {
    std::string recv_partial_e_share_vec_str;
    ps.recv_long_message_from_worker(wk_index, recv_partial_e_share_vec_str);
    std::vector<double> partial_e_share_vec;
    deserialize_double_array(partial_e_share_vec, recv_partial_e_share_vec_str);
    for (double v : partial_e_share_vec) {
      e_share_vec.push_back(v);
    }
  }

  log_info("[WPCC_PS]: 5. all done ");

  // after getting e, calculate q2, y_vec_min_mean_vec in form of 1*1
  auto **y_vec_min_mean_vec = new EncodedNumber *[1];
  y_vec_min_mean_vec[0] = new EncodedNumber[1];

  // calculate [y_i] - [mean_y_cipher] in form of N*1
  auto **y_vec_min_mean_y_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    EncodedNumber tmp_res;
    djcs_t_aux_ee_add_ext(phe_pub_key, tmp_res, mean_y_cipher_neg, predictions[i]);
    y_vec_min_mean_y_cipher[i] = new EncodedNumber[1];
    y_vec_min_mean_y_cipher[i][0] = tmp_res;
  }

  int y_min_mean_y_cipher_pre = std::abs(y_vec_min_mean_y_cipher[0][0].getter_exponent());
  log_info("[WPCC_PS]: 6 calculate [y] - [mean_y], result's precision ="
               + std::to_string(y_min_mean_y_cipher_pre));

  // 6. calculate q2
  // make e_share to two dim,
  two_d_e_share_vec.push_back(e_share_vec);
  cipher_shares_mat_mul(ps.party,
                        two_d_e_share_vec, // 1*N
                        y_vec_min_mean_y_cipher, // N * 1
                        1, num_instance, num_instance, 1,
                        y_vec_min_mean_vec);

  // convert q2 into share
  // in form of 1*N
  ciphers_to_secret_shares(ps.party, y_vec_min_mean_vec[0],
                           q2_shares,
                           1,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[WPCC_PS]: 7. convert q2 cipher and convert it to share done");

//  // init result featureNum * 1
//  auto **party_local_tmp_wf = new EncodedNumber *[ps.party.getter_feature_num()];
//  for (int i = 0; i < ps.party.getter_feature_num(); i++) {
//    party_local_tmp_wf[i] = new EncodedNumber[1];
//  }

//  // in form of feature_num * sample*num
//  auto **local_encrypted_feature = new EncodedNumber *[ps.party.getter_feature_num()];
//  for (int i = 0; i < ps.party.getter_feature_num(); i++) {
//    local_encrypted_feature[i] = new EncodedNumber[num_instance];
//  }

  log_info("[WPCC_PS]: 8. each party compute F*[W] in parallel done");
  djcs_t_free_public_key(phe_pub_key);

  // clean the code
  delete[] sum_sss_weight_cipher;
  for (int i = 0; i < num_instance; i++) {
    delete[] two_d_prediction_cipher[i];
  }
  delete[] two_d_prediction_cipher;
  delete[] sss_weight_cipher;
  for (int i = 0; i < num_instance; i++) {
    delete[] y_vec_min_mean_y_cipher[i];
  }
  delete[] y_vec_min_mean_y_cipher;
  delete[] two_d_sss_weight_mul_pred_cipher[0];
  delete[] two_d_sss_weight_mul_pred_cipher;
  delete[] mean_y_cipher;
  delete[] y_vec_min_mean_vec[0];
  delete[] y_vec_min_mean_vec;
}

/***********************************************************/
/***************** logic for worker ***************/
/***********************************************************/

void worker_calculate_wpcc_batch_pre_info(const Party &party,
                                          int wk_index,
                                          const std::vector<double> &sss_sample_weights_share,
                                          const std::vector<std::vector<int>> &instance_partition_vec,
                                          EncodedNumber *predictions,
                                          EncodedNumber *mean_y_cipher,
                                          std::vector<double> &partial_e_share_vec) {
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  log_info("[worker] wk_index = " + std::to_string(wk_index));

  // 1 calculate the (<w> * {[y] - [mean_y]}) ** 2
  EncodedNumber mean_y_cipher_neg;
  convert_cipher_to_negative(phe_pub_key, mean_y_cipher[0], mean_y_cipher_neg);
  log_info("[worker]: each party compute -[mean_y]");

  int worker_num_instance = (int) instance_partition_vec[wk_index].size();
  log_info("[worker]: worker_num_instance = " + std::to_string(worker_num_instance));

  std::vector<double> e_share_vec; // N vector
  auto *e_cipher_vec = new EncodedNumber[worker_num_instance];
  for (int index = 0; index < worker_num_instance; index++) {
    // calculate [y_i] - [mean_y_cipher]
    EncodedNumber y_min_mean_y_cipher;
    djcs_t_aux_ee_add_ext(phe_pub_key, y_min_mean_y_cipher,
                          mean_y_cipher_neg, predictions[instance_partition_vec[wk_index][index]]);

    int y_min_mean_y_cipher_pre = std::abs(y_min_mean_y_cipher.getter_exponent());
//    log_info("[pearson_fl]: calculate the [y] - [mean_y], result's precision ="
//                 + std::to_string(y_min_mean_y_cipher_pre));

    // convert [y_i] - [mean_y_cipher] into two dim in form of 1*1
    auto **two_d_mean_y_cipher_neg = new EncodedNumber *[1];
    two_d_mean_y_cipher_neg[0] = new EncodedNumber[1];
    two_d_mean_y_cipher_neg[0][0] = y_min_mean_y_cipher;

    // init result 1*1 metrics
    auto **sss_weight_mul_y_min_meany_cipher = new EncodedNumber *[1];
    sss_weight_mul_y_min_meany_cipher[0] = new EncodedNumber[1];

    // pick one element from two_d_sss_weights_share each time and wrapper it as two-d vector
    std::vector<std::vector<double>> two_d_sss_weights_share_element;
    std::vector<double> tmp_vec;
//    log_info("[worker] sss_sample_weights_share.size = " + std::to_string(sss_sample_weights_share.size()));
//    log_info("[worker] instance_partition_vec[" + std::to_string(wk_index) + "]["
//      + std::to_string(index) + "] = " + std::to_string(instance_partition_vec[wk_index][index]));
    tmp_vec.push_back(sss_sample_weights_share[instance_partition_vec[wk_index][index]]);
    two_d_sss_weights_share_element.push_back(tmp_vec);

    cipher_shares_mat_mul(party,
                          two_d_sss_weights_share_element,
                          two_d_mean_y_cipher_neg, 1, 1, 1, 1,
                          sss_weight_mul_y_min_meany_cipher);

//    log_info("[pearson_fl]: 5.3 convert [y_i] - [mean_y_cipher] into two dim in form of 1*1 ");

    // copy result to e_cipher_vec
    e_cipher_vec[index] = sss_weight_mul_y_min_meany_cipher[0][0];

    // clear the memory
    delete[] two_d_mean_y_cipher_neg[0];
    delete[] two_d_mean_y_cipher_neg;
    delete[] sss_weight_mul_y_min_meany_cipher[0];
    delete[] sss_weight_mul_y_min_meany_cipher;
  }

  // decrypt and get es share
  ciphers_to_secret_shares(party,
                           e_cipher_vec,
                           e_share_vec,
                           worker_num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  partial_e_share_vec = e_share_vec;

  log_info("[pearson_fl]: 6. decrypt finished.");

  delete[] e_cipher_vec;
  djcs_t_free_public_key(phe_pub_key);
}

void worker_calculate_wpcc_per_feature(const Party &party,
                                       const vector<std::vector<double>> &train_data,
                                       const vector<double> &sss_sample_weight_share,
                                       const std::vector<double> &sum_sss_weight_share,
                                       const std::vector<vector<double>> &two_d_e_share_vec,
                                       const std::vector<std::vector<double>> &two_d_sss_weights_share,
                                       const std::vector<double> &q2_shares,
                                       std::vector<double> &wpcc_vec,
                                       const std::vector<int> &global_feature_partition_ids,
                                       const std::vector<int> &global_partyid_look_up_vec,
                                       const std::vector<int> &global_party_local_feature_id_look_up_vec) {

  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  int num_instance = (int) train_data.size();

  // 2. jointly convert <w> into ciphertext [w],
  auto *sss_weight_cipher = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(party,
                           sss_weight_cipher,
                           sss_sample_weight_share,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);

  std::vector<std::vector<int>> worker_party_feature_ids;
  for (int pid = 0; pid < party.party_num; pid++) {
    std::vector<int> party_feature_ids;
    for (int g_f_id : global_feature_partition_ids) {
      int party_id = global_partyid_look_up_vec[g_f_id];
      if (party_id == pid) {
        // is local feature, put into local_feature_ids
        party_feature_ids.push_back(g_f_id);
      }
    }
    worker_party_feature_ids.push_back(party_feature_ids);
  }

  std::vector<int> worker_local_feature_ids = worker_party_feature_ids[party.party_id];
  int worker_local_feature_num = (int) worker_local_feature_ids.size();
  int global_feature_partition_num = (int) global_feature_partition_ids.size();

  log_info("[pearson_fl]: inside worker, worker_local_feature_num = "
               + std::to_string(worker_local_feature_num)
               + ", local feature ids=  " + vec_to_str<int>(worker_local_feature_ids)
               + " global_feature_partition_num = " + std::to_string(global_feature_partition_num));

  // party calculate
  auto **party_local_tmp_wf = new EncodedNumber *[party.getter_feature_num()];
  for (int i = 0; i < party.getter_feature_num(); i++) {
    party_local_tmp_wf[i] = new EncodedNumber[1];
  }
  // init [F] list
  auto **local_encrypted_feature = new EncodedNumber *[party.getter_feature_num()];
  for (int i = 0; i < party.getter_feature_num(); i++) {
    local_encrypted_feature[i] = new EncodedNumber[num_instance];
  }
  // init
  log_info("[pearson_fl]: Party begin to compute F*[W] and [F] in parallel");

  auto **feature_vector_plains = new EncodedNumber *[worker_local_feature_num];
  auto *feature_multiply_w_cipher = new EncodedNumber[worker_local_feature_num];

  for (int i = 0; i < worker_local_feature_num; i++) {
    feature_vector_plains[i] = new EncodedNumber[num_instance];
//    feature_multiply_w_cipher[i] = new EncodedNumber[1];
  }

  // each worker parallel calculate it.
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int worker_f_id = 0; worker_f_id < worker_local_feature_num; worker_f_id++) {
    // local = 2
    // global  =6
    // feature_global_id = [8.9]  [18,19]  [28, 29]
    int feature_global_id = worker_local_feature_ids[worker_f_id];
    int feature_worker_id = find_idx_in_vec(global_feature_partition_ids, feature_global_id);
    // each party has it 8,9
    int feature_id = global_party_local_feature_id_look_up_vec[feature_global_id];
    log_info("[pearson_fl] feature_global_id = " + std::to_string(feature_global_id));
    log_info("[pearson_fl] feature_worker_id = " + std::to_string(feature_worker_id));
    log_info("[pearson_fl] feature_id = " + std::to_string(feature_id));

    // 1. each party compute F*[W]
    log_info("[pearson_fl]: encode training dataset");
    for (int sample_id = 0; sample_id < num_instance; sample_id++) {
      // assign value
      feature_vector_plains[worker_f_id][sample_id].set_double(phe_pub_key->n[0],
                                                               train_data[sample_id][feature_id],
                                                               PHE_FIXED_POINT_PRECISION);
    }
    // calculate F*[W], 1*1
    log_info("[pearson_fl]: calculate F*[W], 1*1");
    djcs_t_aux_inner_product(phe_pub_key,
                             party.phe_random,
                             feature_multiply_w_cipher[worker_f_id],
                             sss_weight_cipher,
                             feature_vector_plains[worker_f_id],
                             num_instance);
    party_local_tmp_wf[feature_id][0] = feature_multiply_w_cipher[worker_f_id];

    log_info("[pearson_fl]: each party also encrypt the feature vector ");
    // each party also encrypt the feature vector first
    for (int sample_id = 0; sample_id < num_instance; sample_id++) {
      local_encrypted_feature[feature_id][sample_id].set_double(phe_pub_key->n[0],
                                                                train_data[sample_id][feature_id],
                                                                PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key,
                         party.phe_random,
                         local_encrypted_feature[feature_id][sample_id],
                         local_encrypted_feature[feature_id][sample_id]);
    }

  }


  // optimize the worker feature calculation
  // o1. identify the local features that need to calculate
  // o2. for the original step 9.5, each party execute locally and use omp parallelism
  // o3. for the rest steps, the parties jointly compute and update

  int worker_global_feature_num = (int) global_feature_partition_ids.size();
  log_info("[pearson_fl]: worker_global_feature_num = " + std::to_string(worker_global_feature_num));

  auto *mean_f_cipher_vec = new EncodedNumber[worker_global_feature_num];
  auto *squared_mean_f_cipher_vec = new EncodedNumber[worker_global_feature_num];

  auto *p_cipher_vec = new EncodedNumber[worker_global_feature_num];
  for (int g_f_id: global_feature_partition_ids) {

    int party_id = global_partyid_look_up_vec[g_f_id];
    int feature_id = global_party_local_feature_id_look_up_vec[g_f_id];
    int feature_worker_id = find_idx_in_vec(global_feature_partition_ids, g_f_id);

    log_info("[pearson_fl]: DEBUG. current worker calculate global_feature_id = "
                 + std::to_string(g_f_id)
                 + " party_id = " + std::to_string(party_id)
                 + " feature_id = " + std::to_string(feature_id)
    );

    // only the current party have this value. other parties have empty, 1*1
    auto *feature_multiply_w_cipher = new EncodedNumber[1];
    if (party.party_id == party_id) {
      log_info("[pearson_fl]: DEBUG. party_id compute local feature = " +
          std::to_string(party_id)
      );
      feature_multiply_w_cipher[0] = party_local_tmp_wf[feature_id][0];
    }

    // all party jointly compute the get [mean_f], <mean_f>, [-mean_f] and (mean_f)**2
    // get [mean_f], <mean_f>, [-mean_f]
    auto *mean_f_cipher = new EncodedNumber[1];
    std::vector<double> mean_f_share = WeightedMean(party,
                                                    sum_sss_weight_share[0],
                                                    feature_multiply_w_cipher,
                                                    mean_f_cipher,
                                                    party_id);
    EncodedNumber mean_f_cipher_neg;
    convert_cipher_to_negative(phe_pub_key, mean_f_cipher[0], mean_f_cipher_neg);

    // get (mean_f)**2
    auto *squared_mean_f_cipher = new EncodedNumber[1];
    ciphers_ele_wise_multi(party, squared_mean_f_cipher, mean_f_cipher, mean_f_cipher,
                           1, party_id);
    mean_f_cipher_vec[feature_worker_id] = mean_f_cipher[0];
    squared_mean_f_cipher_vec[feature_worker_id] = squared_mean_f_cipher[0];
    log_info("[pearson_fl]: 9. all parties compute mean_f and (mean_f)**2");

    // only the current party calculate [F] - [mean_F], other don;t have value of f_vec_min_mean_f_cipher
    // N*1
    auto **f_vec_min_mean_f_cipher = new EncodedNumber *[num_instance];
    for (int sample_id = 0; sample_id < num_instance; sample_id++) {
      f_vec_min_mean_f_cipher[sample_id] = new EncodedNumber[1];
    }

    if (party.party_id == party_id) {
      EncodedNumber f_min_mean_f_cipher;
      for (int sample_id = 0; sample_id < num_instance; sample_id++) {
        djcs_t_aux_ee_add(phe_pub_key,
                          f_min_mean_f_cipher,
                          mean_f_cipher_neg,
                          local_encrypted_feature[feature_id][sample_id]
        );
        f_vec_min_mean_f_cipher[sample_id][0] = f_min_mean_f_cipher;
      }
    }
    log_info("[pearson_fl]: 9.1. calculate [f_i] - [mean_f_cipher],");

    // the current party send it to all other parties for future computation
    broadcast_encoded_number_matrix(party, f_vec_min_mean_f_cipher, num_instance,
                                    1, party_id);
    log_info("[pearson_fl]: 9.2. party id " +
        std::to_string(party_id) + " send [F]-[MeanF] to others");

    // all parties jointly calculate sum(<w_i> * ([f_i] - [mean_F])([y_i] - [mean_Y])), 1*1
    auto **p_cipher = new EncodedNumber *[1];
    p_cipher[0] = new EncodedNumber[1];
    cipher_shares_mat_mul(party,
                          two_d_e_share_vec, // 1*N
                          f_vec_min_mean_f_cipher, // N*1
                          1, num_instance, num_instance, 1,
                          p_cipher);
    log_info("[pearson_fl]: 9.3. all parties jointly calculate sum(<w_i> * ([f_i] - [mean_F])([y_i] - [mean_Y])),");

    p_cipher_vec[feature_worker_id] = p_cipher[0][0];

    delete[] feature_multiply_w_cipher;
    delete[] mean_f_cipher;
    delete[] squared_mean_f_cipher;
    for (int i = 0; i < num_instance; i++) {
      delete[] f_vec_min_mean_f_cipher[i];
    }
    delete[] f_vec_min_mean_f_cipher;
    delete[] p_cipher[0];
    delete[] p_cipher;
  }

  std::vector<double> p_shares_vec; // size = worker_global_feature_num
  //  all parties jointly compute p share, 1*1
  ciphers_to_secret_shares(party, p_cipher_vec,
                           p_shares_vec,
                           worker_global_feature_num,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[pearson_fl]: 9.4. all parties jointly calculate p share <p>");
  delete[] p_cipher_vec;

  // calculate q1 = (f-mean_f) ** 2, tmp_vec_cipher: N*M, where N is instance number and M is feature num
  auto **tmp_vec_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    tmp_vec_cipher[i] = new EncodedNumber[worker_global_feature_num];
  }

  auto **squared_feature_value_cipher_mat = new EncodedNumber *[num_instance];
  auto **middle_value_mat = new EncodedNumber *[num_instance];
  auto **neg_2f_mat = new EncodedNumber *[num_instance];
  auto **tmp_vec_ele_mat = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    squared_feature_value_cipher_mat[i] = new EncodedNumber[worker_local_feature_num];
    middle_value_mat[i] = new EncodedNumber[worker_local_feature_num];
    neg_2f_mat[i] = new EncodedNumber[worker_local_feature_num];
    tmp_vec_ele_mat[i] = new EncodedNumber[worker_local_feature_num];
  }

  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int worker_f_id = 0; worker_f_id < worker_local_feature_num; worker_f_id++) {
    int feature_global_id = worker_local_feature_ids[worker_f_id];
    int feature_worker_id = find_idx_in_vec(global_feature_partition_ids, feature_global_id);
    int feature_id = global_party_local_feature_id_look_up_vec[feature_global_id];
    for (int i = 0; i < num_instance; i++) {
      // calculate [f**2]
      // EncodedNumber squared_feature_value_cipher;
      squared_feature_value_cipher_mat[i][worker_f_id].set_double(phe_pub_key->n[0],
                                                                  train_data[i][feature_id] * train_data[i][feature_id],
                                                                  PHE_FIXED_POINT_PRECISION
                                                                      * PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key,
                         party.phe_random,
                         squared_feature_value_cipher_mat[i][worker_f_id],
                         squared_feature_value_cipher_mat[i][worker_f_id]);

      // calculate -2f*[mean_f]`
      // EncodedNumber middle_value;
      // EncodedNumber neg_2f;
      neg_2f_mat[i][worker_f_id].set_double(phe_pub_key->n[0], -2 * train_data[i][feature_id],
                                            abs(mean_f_cipher_vec[feature_worker_id].getter_exponent()));
      djcs_t_aux_ep_mul(phe_pub_key,
                        middle_value_mat[i][worker_f_id],
                        mean_f_cipher_vec[feature_worker_id],
                        neg_2f_mat[i][worker_f_id]);

      // calculate [f_j]**2 + -2f_j*[mean_F] + [mean_F]**2
      // EncodedNumber tmp_vec_ele;
      djcs_t_aux_ee_add_ext(phe_pub_key,
                            tmp_vec_ele_mat[i][worker_f_id],
                            squared_feature_value_cipher_mat[i][worker_f_id],
                            middle_value_mat[i][worker_f_id]);
      djcs_t_aux_ee_add_ext(phe_pub_key,
                            tmp_vec_ele_mat[i][worker_f_id],
                            tmp_vec_ele_mat[i][worker_f_id],
                            squared_mean_f_cipher_vec[feature_worker_id]);
      tmp_vec_cipher[i][feature_worker_id] = tmp_vec_ele_mat[i][worker_f_id];

    }
  }

  // clear the memory
  for (int i = 0; i < num_instance; i++) {
    delete[] squared_feature_value_cipher_mat[i];
    delete[] middle_value_mat[i];
    delete[] neg_2f_mat[i];
    delete[] tmp_vec_ele_mat[i];
  }
  delete[] squared_feature_value_cipher_mat;
  delete[] middle_value_mat;
  delete[] neg_2f_mat;
  delete[] tmp_vec_ele_mat;

  log_info("[pearson_fl]: 9.5. for each instance, calculate [f**2] + -2f*[mean_f] + [mean_f**2]");

  // the parties need to sync up the tmp_vec_cipher matrix to make sure they have the same matrix
  // each party send the partial tmp_vec_cipher to active party, the active party aggregate and broadcast
  if (party.party_id == falcon::ACTIVE_PARTY) {
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_tmp_vec_ciphers_str;
        party.recv_long_message(id, recv_tmp_vec_ciphers_str);
        auto **recv_tmp_vec_ciphers = new EncodedNumber *[num_instance];
        for (int i = 0; i < num_instance; i++) {
          recv_tmp_vec_ciphers[i] = new EncodedNumber[worker_global_feature_num];
        }
        deserialize_encoded_number_matrix(recv_tmp_vec_ciphers,
                                          num_instance,
                                          worker_global_feature_num,
                                          recv_tmp_vec_ciphers_str);
        // copy party id's results to tmp_vec_ciphers (need to find the corresponding index of party id in worker_global_feature_num vector
        std::vector<int> party_feature_ids = worker_party_feature_ids[id];
        std::vector<int> feature_idx_in_worker_global;
        for (int j = 0; j < party_feature_ids.size(); j++) {
          int idx = find_idx_in_vec(global_feature_partition_ids, party_feature_ids[j]);
          feature_idx_in_worker_global.push_back(idx);
        }

        for (int i = 0; i < num_instance; i++) {
          for (int feature_id : feature_idx_in_worker_global) {
            tmp_vec_cipher[i][feature_id] = recv_tmp_vec_ciphers[i][feature_id];
          }
        }

        for (int i = 0; i < num_instance; i++) {
          delete[] recv_tmp_vec_ciphers[i];
        }
        delete[] recv_tmp_vec_ciphers;
      }
    }
  } else {
    std::string tmp_vec_ciphers_str;
    serialize_encoded_number_matrix(tmp_vec_cipher, num_instance, worker_global_feature_num, tmp_vec_ciphers_str);
    party.send_long_message(ACTIVE_PARTY_ID, tmp_vec_ciphers_str);
  }

  broadcast_encoded_number_matrix(party, tmp_vec_cipher, num_instance, worker_global_feature_num, ACTIVE_PARTY_ID);
  log_info("[pearson_fl]: 9.6. party id " + std::to_string(party.party_id) + " send tmp_vec_cipher to others");

  if (worker_global_feature_num != 0) {
    // all parties jointly calculate q1 cipher and convert it to shares 1*1
    auto **q1_cipher = new EncodedNumber *[1];
    q1_cipher[0] = new EncodedNumber[worker_global_feature_num];
    cipher_shares_mat_mul(party,
                          two_d_sss_weights_share, // 1*N
                          tmp_vec_cipher, // N*M
                          1, num_instance, num_instance, worker_global_feature_num,
                          q1_cipher);
    log_info("[pearson_fl]: 9.7. calculate q1 ");

    // convert cipher to shares
    std::vector<double> q1_shares_vec;
    ciphers_to_secret_shares(party, q1_cipher[0],
                             q1_shares_vec,
                             worker_global_feature_num,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // all parties jointly calculate  WPCC share, wpcc = <p> / (<q1> * <q2>) for this features
    std::vector<double> res_wpcc_shares = compute_wpcc(party, p_shares_vec, q1_shares_vec, q2_shares[0]);
    log_info("[pearson_fl]: 9.8. calculate WPCC finished");
    for (double v : res_wpcc_shares) {
      wpcc_vec.push_back(v);
    }
    delete[] q1_cipher[0];
    delete[] q1_cipher;
  }

  delete[] mean_f_cipher_vec;
  delete[] squared_mean_f_cipher_vec;
  for (int i = 0; i < num_instance; i++) {
    delete[] tmp_vec_cipher[i];
  }
  delete[] tmp_vec_cipher;

  for (int i = 0; i < party.getter_feature_num(); i++) {
    delete[] party_local_tmp_wf[i];
  }
  delete[] party_local_tmp_wf;
  for (int i = 0; i < party.getter_feature_num(); i++) {
    delete[] local_encrypted_feature[i];
  }
  delete[] local_encrypted_feature;
  delete[] sss_weight_cipher;

  for (int i = 0; i < worker_local_feature_num; i++) {
    delete[] feature_vector_plains[i];
  }
  delete[] feature_vector_plains;
  delete[] feature_multiply_w_cipher;

  djcs_t_free_public_key(phe_pub_key);

  log_info("[pearson_fl]: 10. All done, begin to clear the memory");
}


/***********************************************************/
/***************** plaintext for verfication ***************/
/***********************************************************/

std::vector<double> get_local_features_correlations_plaintext(const Party &party,
                                                              const std::vector<int> &party_feature_nums,
                                                              const vector<std::vector<double>> &train_data,
                                                              EncodedNumber *predictions,
                                                              const vector<double> &sss_sample_weights_share,
                                                              std::vector<double> &wpcc_vec) {
  // 1. init, get batch_true_labels_precision
  int batch_true_labels_precision = std::abs(predictions[0].getter_exponent());
  log_info("[wpcc_plaintext]: batch_true_labels_precision = " + std::to_string(batch_true_labels_precision));

  // get number of instance
  // 2. jointly convert <w> into ciphertext [w], and saved in active party
  int num_instance = train_data.size();
  log_info("[features_correlations_plaintext]: number of num_instance = " + std::to_string(num_instance));

  auto *sss_weight_cipher = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(party,
                           sss_weight_cipher,
                           sss_sample_weights_share,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  // 3. convert cipher to plain
  auto *sss_weight_plain_encoded_num = new EncodedNumber[num_instance];
  collaborative_decrypt(party,
                        sss_weight_cipher,
                        sss_weight_plain_encoded_num,
                        num_instance,
                        ACTIVE_PARTY_ID);
  log_info("[wpcc_plaintext]: convert weight cipher to weight plain");

  // 4. convert plain to double
  std::vector<double> instance_weight_double;
  for (int i = 0; i < num_instance; i++) {
    double weight;
    sss_weight_plain_encoded_num[i].decode(weight);
    instance_weight_double.push_back(weight);
  }
  log_info("[wpcc_plaintext]: convert weight plain to weight double");

  double sum_weight = 0;
  for (int i = 0; i < num_instance; i++) {
    sum_weight += instance_weight_double[i];
  }

  // 5.  convert cipher to plain and double
  auto *prediction_plain_encoded_num = new EncodedNumber[num_instance];
  collaborative_decrypt(party,
                        predictions,
                        prediction_plain_encoded_num,
                        num_instance,
                        ACTIVE_PARTY_ID);
  std::vector<double> prediction_plain_double;
  for (int i = 0; i < num_instance; i++) {
    double prediction_double;
    prediction_plain_encoded_num[i].decode(prediction_double);
    prediction_plain_double.push_back(prediction_double);
  }
  log_info("[wpcc_plaintext]: convert prediction cipher to prediction double");
  std::vector<double> wpcc_plain_double;

  // all party send local feature to active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // init the full_train_data with train_data
    vector<std::vector<double>> full_train_data = train_data;
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        // receive from other's party
        std::string receive_local_dataset_str;
        party.recv_long_message(id, receive_local_dataset_str);
        vector<std::vector<double>> received_train_data;
        deserialize_double_matrix(received_train_data, receive_local_dataset_str);
        // attach the dataset locally
        for (int sample_id = 0; sample_id < received_train_data.size(); sample_id++) {
          for (int feature_id = 0; feature_id < received_train_data[sample_id].size(); feature_id++) {
            double record_feature_double = received_train_data[sample_id][feature_id];
            full_train_data[sample_id].push_back(record_feature_double);
          }
        }
      }
    }

    log_info("[wpcc_plaintext]: active party receive all parties' datasets, print the datasets as following");
    for (auto &each_record : full_train_data) {
      std::stringstream ss;
      for (size_t i = 0; i < each_record.size(); i++) {
        if (i != 0) {
          ss << ", ";
        }
        ss << each_record[i];
      }
      log_info("checking training datasets:" + ss.str());
    }

    int total_feature_num = full_train_data[0].size();
    // active party have full training data now, calculate WPCC for each feature.

    // 1. calculate mean Y
    double mean_y = 0;
    for (int sample_id = 0; sample_id < full_train_data.size(); sample_id++) {
      mean_y += instance_weight_double[sample_id] * prediction_plain_double[sample_id];
    }

    mean_y = mean_y / sum_weight;
    log_info("[wpcc_plaintext]: active party calculate mean Y = " + std::to_string(mean_y));

    for (int feature_id = 0; feature_id < total_feature_num; feature_id++) {
      // get mean_f
      double mean_f = 0;
      for (int sample_id = 0; sample_id < full_train_data.size(); sample_id++) {
        mean_f += instance_weight_double[sample_id] * full_train_data[sample_id][feature_id];
      }
      mean_f = mean_f / sum_weight;

      // calculate p, q1, q2
      double numerator = 0;
      double denominator_q1 = 0;
      double denominator_q2 = 0;
      for (int sample_id = 0; sample_id < full_train_data.size(); sample_id++) {
        numerator += instance_weight_double[sample_id]
            * (prediction_plain_double[sample_id] - mean_y)
            * (full_train_data[sample_id][feature_id] - mean_f);
        denominator_q1 += instance_weight_double[sample_id]
            * (full_train_data[sample_id][feature_id] - mean_f)
            * (full_train_data[sample_id][feature_id] - mean_f);
        denominator_q2 += instance_weight_double[sample_id]
            * (prediction_plain_double[sample_id] - mean_y)
            * (prediction_plain_double[sample_id] - mean_y);
      }
      double wpcc = numerator / (sqrt(denominator_q1) * sqrt(denominator_q2));
      wpcc_plain_double.push_back(wpcc);
      log_info("[wpcc_plaintext]: DEBUG. feature index = " + std::to_string(feature_id)
                   + " wpcc = " + std::to_string(wpcc)
                   + " p = " + std::to_string(numerator)
                   + " q1 = " + std::to_string(denominator_q1)
                   + " q2 = " + std::to_string(denominator_q2)
      );
    }
  } else {
    // 1.send to active party
    std::string local_dataset_str;
    serialize_double_matrix(train_data, local_dataset_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_dataset_str);
  }

  delete[] sss_weight_cipher;
  delete[] sss_weight_plain_encoded_num;
  delete[] prediction_plain_encoded_num;

  // convert all to positive
  for (double &element : wpcc_plain_double) {
    if (element <= 0) {
      element = -element;
    }
  }

  return wpcc_plain_double;
}

std::vector<double> jointly_get_top_k_features_plaintext(const Party &party,
                                                         const std::vector<int> &party_feature_nums,
                                                         const std::vector<double> &feature_cor_shares,
                                                         const std::vector<int> &party_id_loop_ups,
                                                         const std::vector<int> &party_feature_id_look_ups,
                                                         int num_explained_features) {
  int total_feature_num = 0;
  for (auto &ele: party_feature_nums) {
    total_feature_num += ele;
  }

  log_info("[top_k_features_index]: DEBUG. size of wpcc share = " + std::to_string(feature_cor_shares.size()) +
      " size of total feature = " + std::to_string(total_feature_num));

  std::vector<double> feature_corr_plain_double;
  secret_shares_to_plain_double(party, feature_corr_plain_double, feature_cor_shares, total_feature_num,
                                ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  for (int i = 0; i < total_feature_num; i++) {
    log_info("[top_k_features_index]: DEBUG. feature index = " + std::to_string(i) + " wpcc = "
                 + std::to_string(feature_corr_plain_double[i]));
  }

  // convert all to positive
  for (double &element : feature_corr_plain_double) {
    if (element <= 0) {
      element = -element;
    }
  }

  std::vector<int> global_indexs = index_of_top_k_in_vector(feature_corr_plain_double, num_explained_features);

  // Print the indices of the largest k elements
  for (auto g_index: global_indexs) {
    log_info("[top_k_features_index]: DEBUG. global feature id " + std::to_string(g_index)
                 + " party id = " + std::to_string(party_id_loop_ups[g_index])
                 + " party's local feature index = " + std::to_string(party_feature_id_look_ups[g_index])
    );
  }

  return feature_corr_plain_double;
}


