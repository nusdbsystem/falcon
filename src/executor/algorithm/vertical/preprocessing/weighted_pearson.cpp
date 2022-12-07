//
// Created by naili on 21/8/22.
//

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson.h"
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>
#include <future>

std::vector<double> WeightedPearson::get_feature_importance(Party party,
                                                     int class_id,
                                                     const vector<std::vector<double>> &train_data,
                                                     EncodedNumber *predictions,
                                                     const vector<double> &sss_sample_weights,
                                                     const string &ps_network_str,
                                                     int is_distributed,
                                                     int distributed_role,
                                                     int worker_id) {
  // init local explanation result
  std::vector<double> local_explanations;

  // get datasets
  std::vector<std::vector<double>> used_train_data = train_data;
  int weight_size = (int) used_train_data[0].size();
  party.setter_feature_num(weight_size);
  log_info("start weighted pearson");

  local_explanations = get_correlation(party, class_id,train_data, predictions, sss_sample_weights);

  return local_explanations;
}


std::vector<double> WeightedPearson::get_correlation(Party party,
                                                     int class_id,
                                                     const vector<std::vector<double>> &train_data,
                                                     EncodedNumber *predictions,
                                                     const vector<double> &sss_sample_weights) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();


  // convert to 2D metrics.
  std::vector<std::vector<double>> two_d_weights;
  two_d_weights.push_back(sss_sample_weights);

  // result
  std::vector<double> wpcc_vec;

  // 1. calculate weight_sum



  // 1. jointly convert <w> into ciphertext [w], and saved in active party
  int num_instance = train_data.size();
  auto* cipher_instance_weight = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(party,
                           cipher_instance_weight,
                           sss_sample_weights,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);


  // 2. activate party get the sum and convert it to secret shares, and each party hold one share
  log_info("step 2, getting the sum of weight");
  EncodedNumber *sum_w = &cipher_instance_weight[0];
  for (int i = 1; i < num_instance; i++ ){
    djcs_t_aux_ee_add(phe_pub_key, *sum_w, *sum_w, cipher_instance_weight[i]);
  }

  std::vector<double> share_sum_w;
  ciphers_to_secret_shares(party, sum_w, share_sum_w, num_instance ,ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);


  // each party will have this after first phase,
  // todo: receive and send this value /
  std::vector< vector<double>> two_d_e_share_vec;

  // 3. active party compute mean_Y and send it to others.
  if (party.party_type == falcon::ACTIVE_PARTY) {
    log_info("step 2, encrypted label to [y]");

    auto** encrypted_label = new EncodedNumber*[num_instance];
    for (int i = 0; i < num_instance; i++) {
      encrypted_label[i] = new EncodedNumber[1];
    }
    for (int i = 0; i < num_instance; i++){
      encrypted_label[i][0] = predictions[i];
    }

    auto** ciphers_shares_mul_res = new EncodedNumber*[1];
    for (int i = 0; i < 1; i++) {
      ciphers_shares_mul_res[i] = new EncodedNumber[1];
    }


    // active party calculate WY and convert it to secret share
    cipher_shares_mat_mul(party,
                          two_d_weights,
                          encrypted_label, 1, num_instance, num_instance, 1,
                          ciphers_shares_mul_res);

    std::vector<double> ciphers_shares_mul_shares;
    ciphers_to_secret_shares(party, ciphers_shares_mul_res[0],
                             ciphers_shares_mul_shares,
                             num_instance,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // active party compute mean y with [mean_y] = SSS2PHE(<tmp> / <w_sum>)
    std::vector<double> mean_y_share;
    std::vector<int> public_values;
//    public_values.push_back(sample_size);
//    public_values.push_back(total_feature_size);
    falcon::SpdzLimeCompType comp_type = falcon::PEARSON;
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values = promise_values.get_future();
    std::thread spdz_dist_weights(spdz_lime_divide,
                                  party.party_num,
                                  party.party_id,
                                  party.executor_mpc_ports,
                                  party.host_names,
                                  public_values.size(),
                                  public_values,
                                  ciphers_shares_mul_shares.size(),
                                  ciphers_shares_mul_shares,
                                  comp_type,
                                  &promise_values);
    std::vector<double> res = future_values.get();
    spdz_dist_weights.join();
    mean_y_share = res;
    log_info("[compute_mean_y]: communicate with spdz finished");
    log_info("[compute_mean_y]: res.size = " + std::to_string(res.size()));

    // convert mean_y share into mean_y cipher
    auto* mean_y_cipher = new EncodedNumber[1];
    secret_shares_to_ciphers(party,
                             mean_y_cipher,
                             mean_y_share,
                             num_instance,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // calculate the second item of denominator in equ 11.
    // this should be the 1*N vector
    std::vector<double> e_share_vec;
    EncodedNumber neg_one_int;
    neg_one_int.set_integer(phe_pub_key->n[0], -1);
    EncodedNumber mean_y_cipher_neg;
    djcs_t_aux_ep_mul(phe_pub_key, mean_y_cipher_neg, mean_y_cipher[0], neg_one_int);
    for (int index = 0; index < num_instance; index ++){
      // calculate [y_i] - [mean_y_cipher]
      EncodedNumber y_min_mean_y_cipher;
      djcs_t_aux_ee_add(phe_pub_key, y_min_mean_y_cipher, mean_y_cipher_neg, predictions[index]);

      auto** two_d_mean_y_cipher_neg = new EncodedNumber*[1];
      for (int i = 0; i < 1; i++) {
        two_d_mean_y_cipher_neg[i] = &y_min_mean_y_cipher;
      }

      auto** cipher_y_min_mean = new EncodedNumber*[1];
      for (int i = 0; i < 1; i++) {
        cipher_y_min_mean[i] = new EncodedNumber[1];
      }

      cipher_shares_mat_mul(party,
                            two_d_weights,
                            two_d_mean_y_cipher_neg, 1, 1, 1, 1,
                            cipher_y_min_mean);


      // get es share
      std::vector<double> es_share;
      ciphers_to_secret_shares(party, cipher_y_min_mean[0],
                               es_share,
                               1,
                               ACTIVE_PARTY_ID,
                               PHE_FIXED_POINT_PRECISION);
      e_share_vec.push_back(es_share[0]);
    }

    // after getting e, calculate q2
    auto** y_vec_min_mean_vec = new EncodedNumber*[1];
    for (int i = 0; i < 1; i++) {
      y_vec_min_mean_vec[i] = new EncodedNumber[1];
    }
    // calculate [y_i] - [mean_y_cipher]
    auto** y_vec_min_mean_y_cipher = new EncodedNumber*[num_instance];
    for ( int i = 0; i < num_instance; i++){
      EncodedNumber tmp_res;
      djcs_t_aux_ee_add(phe_pub_key, tmp_res, mean_y_cipher_neg, predictions[i]);
      y_vec_min_mean_y_cipher[i] = &tmp_res;
    }

    two_d_e_share_vec.push_back(e_share_vec);
    cipher_shares_mat_mul(party,
                          two_d_e_share_vec,
                          y_vec_min_mean_y_cipher, 1, num_instance, num_instance, 1,
                          y_vec_min_mean_vec);


    std::vector<double> q2_shares;
    ciphers_to_secret_shares(party, y_vec_min_mean_vec[0],
                             q2_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);
  }

  // each party calculate local feature correlation

  // 1. calculate local feature mean
  for (int feature_id = 0; feature_id < party.getter_feature_num(); feature_id++){
    auto** feature_vector_plains = new EncodedNumber*[num_instance];
    for (int sample_id = 0; sample_id < num_instance; sample_id ++){
      feature_vector_plains[0] = new EncodedNumber[1];
      feature_vector_plains[0]->set_double(phe_pub_key->n[0],
                                           train_data[sample_id][feature_id],
                                           PHE_FIXED_POINT_PRECISION);
    }
    auto* feature_multiply_w_cipher = new EncodedNumber[1];
    djcs_t_aux_vec_mat_ep_mult(phe_pub_key,
                               party.phe_random,
                               feature_multiply_w_cipher,
                               cipher_instance_weight,
                               feature_vector_plains,
                               1,
                               num_instance);

    // convert cipher to shares
    std::vector<double> tmp_shares;
    ciphers_to_secret_shares(party, feature_multiply_w_cipher,
                             tmp_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // calculate mean of mu
    std::vector<double> mean_f_share;
    std::vector<int> public_values;
    //    public_values.push_back(sample_size);
    //    public_values.push_back(total_feature_size);
    falcon::SpdzLimeCompType comp_type = falcon::PEARSON;
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values = promise_values.get_future();
    std::thread spdz_dist_weights(spdz_lime_divide,
                                  party.party_num,
                                  party.party_id,
                                  party.executor_mpc_ports,
                                  party.host_names,
                                  public_values.size(),
                                  public_values,
                                  tmp_shares.size(),
                                  tmp_shares,
                                  comp_type,
                                  &promise_values);
    std::vector<double> res = future_values.get();
    spdz_dist_weights.join();
    mean_f_share = res;

    // convert mu back to cipher
    auto* mean_f_cipher = new EncodedNumber[1];
    secret_shares_to_ciphers(party,
                             mean_f_cipher,
                             mean_f_share,
                             num_instance,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);


    // calculate squared mean value f
    std::vector<double> squared_mean_f_share;
    //    public_values.push_back(sample_size);
    //    public_values.push_back(total_feature_size);
    comp_type = falcon::PEARSON;
    std::promise<std::vector<double>> promise_values_squared;
    std::future<std::vector<double>> future_values_squared = promise_values_squared.get_future();
    std::thread spdz_dist_squared(spdz_lime_divide,
                                  party.party_num,
                                  party.party_id,
                                  party.executor_mpc_ports,
                                  party.host_names,
                                  public_values.size(),
                                  public_values,
                                  mean_f_share.size(),
                                  mean_f_share,
                                  comp_type,
                                  &promise_values_squared);
    std::vector<double> res_squared = future_values_squared.get();
    spdz_dist_weights.join();
    squared_mean_f_share = res;


    // convert squared_mean_f back to cipher
    auto* squared_mean_f_cipher = new EncodedNumber[1];
    secret_shares_to_ciphers(party,
                             squared_mean_f_cipher,
                             squared_mean_f_share,
                             num_instance,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // calculate numerator in equ 11
    // encrypt the feature vector first
    auto* encrypted_feature = new EncodedNumber[num_instance];
    for (int i = 0; i < num_instance; i++) {
      encrypted_feature[i].set_double(phe_pub_key->n[0], train_data[i][feature_id], PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_feature[i], encrypted_feature[i]);
    }

    // calculate [f_i] - [mean_f_cipher]
    EncodedNumber neg_one_int;
    neg_one_int.set_integer(phe_pub_key->n[0], -1);
    EncodedNumber mean_f_cipher_neg;
    djcs_t_aux_ep_mul(phe_pub_key, mean_f_cipher_neg, squared_mean_f_cipher[0], neg_one_int);

    auto** f_vec_min_mean_f_cipher = new EncodedNumber*[num_instance];
    for (int index = 0; index < num_instance; index ++){
      EncodedNumber f_min_mean_f_cipher;
      djcs_t_aux_ee_add(phe_pub_key, f_min_mean_f_cipher, mean_f_cipher_neg, encrypted_feature[index]);
      f_vec_min_mean_f_cipher[index] = &f_min_mean_f_cipher;
    }


    auto** f_vec_min_mean_vec_share = new EncodedNumber*[1];
    for (int i = 0; i < 1; i++) {
      f_vec_min_mean_vec_share[i] = new EncodedNumber[1];
    }

    // calculate <w> * ([F] - [mean_F])
    cipher_shares_mat_mul(party,
                          two_d_e_share_vec,
                          f_vec_min_mean_f_cipher, 1, num_instance, num_instance, 1,
                          f_vec_min_mean_vec_share);

    // get p share
    std::vector<double> p_shares;
    ciphers_to_secret_shares(party, f_vec_min_mean_vec_share[0],
                             p_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // calculate (f-mean_f) ** 2
    auto** tmp_vec_cipher = new EncodedNumber*[num_instance];

    for (int i = 0; i < num_instance; i++){
      // calculate [f**2]
      EncodedNumber encrypted_squared_feature_value;
      encrypted_squared_feature_value.set_double(phe_pub_key->n[0],
                                                 train_data[i][feature_id] * train_data[i][feature_id],
                                                 PHE_FIXED_POINT_PRECISION* PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         encrypted_squared_feature_value,
                         encrypted_squared_feature_value);

      // calculate -2f*[mean_f]
      EncodedNumber middle_value;
      EncodedNumber neg_2f;
      double value = -2*train_data[i][feature_id];
      neg_2f.set_integer(phe_pub_key->n[0], value);
      djcs_t_aux_ep_mul(phe_pub_key, middle_value, mean_f_cipher[0], neg_2f);

      // calculate [f**2] + -2f*[mean_f] + [mean_f**2]
      EncodedNumber tmp_vec_ele;
      djcs_t_aux_ee_add(phe_pub_key, tmp_vec_ele, encrypted_squared_feature_value, middle_value);
      djcs_t_aux_ee_add(phe_pub_key, tmp_vec_ele, tmp_vec_ele, squared_mean_f_cipher[0]);

      tmp_vec_cipher[i] = &tmp_vec_ele;
    }


    // calculate q1
    auto** q1_cipher = new EncodedNumber*[1];
    for (int i = 0; i < num_instance; i++) {
      q1_cipher[i] = new EncodedNumber[1];
    }

    cipher_shares_mat_mul(party,
                          two_d_weights,
                          tmp_vec_cipher, 1, num_instance, num_instance, 1,
                          q1_cipher);

    // convert cipher to shares
    std::vector<double> q1_shares;
    ciphers_to_secret_shares(party, q1_cipher[0],
                             q1_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // for this feature, calculate WPCC share

    // calculate squared mean value f
    std::vector<double> wpcc_shares;
    //    public_values.push_back(sample_size);
    //    public_values.push_back(total_feature_size);
    comp_type = falcon::PEARSON;
    std::promise<std::vector<double>> promise_values_wpcc;
    std::future<std::vector<double>> future_values_wpcc = promise_values_wpcc.get_future();
    std::thread spdz_dist_wpcc(spdz_lime_WPCC,
                                  party.party_num,
                                  party.party_id,
                                  party.executor_mpc_ports,
                                  party.host_names,
                                  public_values.size(),
                                  public_values,
                                  q1_shares.size(),
                                  q1_shares,
                                  comp_type,
                                  &promise_values_wpcc);
    std::vector<double> res_wpcc = future_values_wpcc.get();
    spdz_dist_weights.join();
    wpcc_shares = res_wpcc;
    wpcc_vec.push_back(wpcc_shares[0]);
  }

  djcs_t_free_public_key(phe_pub_key);

  return wpcc_vec;
}




void spdz_lime_divide(int party_num,
                      int party_id,
                      std::vector<int> mpc_port_bases,
                      std::vector<std::string> party_host_names,
                      int public_value_size,
                      const std::vector<int>& public_values,
                      int private_value_size,
                      const std::vector<double>& private_values,
                      falcon::SpdzLimeCompType lime_comp_type,
                      std::promise<std::vector<double>> *res) {

}

void spdz_lime_WPCC(int party_num,
                      int party_id,
                      std::vector<int> mpc_port_bases,
                      std::vector<std::string> party_host_names,
                      int public_value_size,
                      const std::vector<int>& public_values,
                      int private_value_size,
                      const std::vector<double>& private_values,
                      falcon::SpdzLimeCompType lime_comp_type,
                      std::promise<std::vector<double>> *res) {

}

