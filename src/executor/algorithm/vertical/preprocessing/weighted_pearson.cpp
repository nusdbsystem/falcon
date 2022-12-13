//
// Created by naili on 21/8/22.
//

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson.h"
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>
#include <future>

std::vector<double> wpcc_feature_selection(Party party,
                                           int class_id,
                                           const vector<std::vector<double>> &train_data,
                                           EncodedNumber *predictions,
                                           const vector<double> &sss_sample_weights,
                                           const string &ps_network_str,
                                           int is_distributed,
                                           int distributed_role,
                                           int worker_id) {
  // 1. init local explanation result, each party will hold a share for each weight.
  std::vector<double> local_explanations;

  // 2. get datasets
  std::vector<std::vector<double>> used_train_data = train_data;
  int weight_size = (int) used_train_data[0].size();
  party.setter_feature_num(weight_size);

  // 3. get the weights for each feature.
  if (is_distributed == 0) {
    log_info("start centralized weighted pearson");
    local_explanations = get_correlation(party, class_id, train_data, predictions, sss_sample_weights);
  }

  if (is_distributed == 1 && distributed_role == falcon::DistPS) {

  }

  if (is_distributed == 1 && distributed_role == falcon::DistWorker) {
    auto worker = new Worker(ps_network_str, worker_id);
  }

  log_info("Pearson Feature Selection Done");

  return local_explanations;
}

std::vector<double> get_correlation(const Party &party,
                                    int class_id,
                                    const vector<std::vector<double>> &train_data,
                                    EncodedNumber *predictions,
                                    const vector<double> &sss_sample_weights_share) {
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

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
  }
  for (int i = 0; i < num_instance; i++) {
    two_d_prediction_cipher[i][0] = predictions[i];
  }

  // 1. init local explanation result, each party will hold a share for each weight.
  std::vector<double> wpcc_vec;

  // 2. jointly convert <w> into ciphertext [w], and saved in active party
  auto *sss_weight_cipher = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(party,
                           sss_weight_cipher,
                           sss_sample_weights_share,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[pearson_fl]: 1. jointly convert <w> into ciphertext [w], and saved in active party");

  // 3. all parties calculate sum, and convert it to share
  log_info("step 2, getting the sum of weight");
  EncodedNumber *sum_sss_weight_cipher = &sss_weight_cipher[0];
  for (int i = 1; i < num_instance; i++) {
    djcs_t_aux_ee_add(phe_pub_key, *sum_sss_weight_cipher, *sum_sss_weight_cipher, sss_weight_cipher[i]);
  }

  // 4. active party convert it to secret shares, and each party hold one share
  std::vector<double> sum_sss_weight_share;
  ciphers_to_secret_shares(party,
                           sum_sss_weight_cipher,
                           sum_sss_weight_share,
                           num_instance,
                           ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  log_info("[pearson_fl]: 2. active party convert sum to secret shares, and each party hold one share");

  // in form of 1*N
  std::vector<vector<double>> two_d_e_share_vec;
  std::vector<double> q2_shares;

  // 3. active party calculate <w>*[Prediction] and convert it to secret share
  // init result two dimension cipher of 1*1
  auto **two_d_sss_weight_mul_pred_cipher = new EncodedNumber *[1];
  for (int i = 0; i < 1; i++) {
    two_d_sss_weight_mul_pred_cipher[i] = new EncodedNumber[1];
  }
  cipher_shares_mat_mul(party,
                        two_d_sss_weights_share,
                        two_d_prediction_cipher, 1, num_instance, num_instance, 1,
                        two_d_sss_weight_mul_pred_cipher);

  log_info("[pearson_fl]: 3. active party calculate <w>*[Prediction] and convert it to secret share");

  // each party hold one share
  std::vector<double> two_d_sss_weight_mul_pred_share;
  ciphers_to_secret_shares(party,
                           two_d_sss_weight_mul_pred_cipher[0],
                           two_d_sss_weight_mul_pred_share,
                           1,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);

  // 4. active party compute mean y with [mean_y] = SSS2PHE(<two_d_sss_weight_mul_pred_share> / <w_sum>)
  // send to MPC
  std::vector<double> private_values;
  private_values.push_back(two_d_sss_weight_mul_pred_share[0]);
  private_values.push_back(sum_sss_weight_share[0]);
  std::vector<double> mean_y_share;
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
  std::vector<double> mean_y_res = future_values_mean_y.get();
  spdz_dist_mean_y.join();
  mean_y_share = mean_y_res;
  log_info("[compute_mean_y]: communicate with spdz finished");
  log_info("[compute_mean_y]: res.size = " + std::to_string(mean_y_res.size()));

  log_info("[pearson_fl]: 4.1 active party compute mean_U = <two_d_sss_weight_mul_pred_share> / <w_sum>)");

  // convert mean_y share into mean_y cipher
  auto *mean_y_cipher = new EncodedNumber[1];
  secret_shares_to_ciphers(party,
                           mean_y_cipher,
                           mean_y_share,
                           1,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);

  log_info("[pearson_fl]: 4.2. active party compute [mean_y] = SSS2PHE(mean_U)");

  // 5. calculate the (<w> * {[y] - [mean_y]}) ** 2
  std::vector<double> e_share_vec; // N vector
  EncodedNumber neg_one_int;
  neg_one_int.set_integer(phe_pub_key->n[0], -1);
  EncodedNumber mean_y_cipher_neg;
  djcs_t_aux_ep_mul(phe_pub_key, mean_y_cipher_neg, mean_y_cipher[0], neg_one_int);
  log_info("[pearson_fl]: 4.3 convert [mean_u] to [-mean_u]");

  for (int index = 0; index < num_instance; index++) {
    // calculate [y_i] - [mean_y_cipher]
    EncodedNumber y_min_mean_y_cipher;
    djcs_t_aux_ee_add_ext(phe_pub_key, y_min_mean_y_cipher, mean_y_cipher_neg, predictions[index]);

    int y_min_mean_y_cipher_pre = std::abs(y_min_mean_y_cipher.getter_exponent());
    log_info("[pearson_fl]: 5.2 calculate the [y] - [mean_y], result's precision ="
                 + std::to_string(y_min_mean_y_cipher_pre));

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

    log_info("[pearson_fl]: 5.3 convert [y_i] - [mean_y_cipher] into two dim in form of 1*1 ");

    // get es share
    std::vector<double> es_share;
    ciphers_to_secret_shares(party,
                             sss_weight_mul_y_min_meany_cipher[0],
                             es_share,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);
    e_share_vec.push_back(es_share[0]);

    // clear the memory
    delete[] two_d_mean_y_cipher_neg[0];
    delete[] two_d_mean_y_cipher_neg;
    delete[] sss_weight_mul_y_min_meany_cipher[0];
    delete[] sss_weight_mul_y_min_meany_cipher;
  }

  log_info("[pearson_fl]: 5. all done ");

  // after getting e, calculate q2, y_vec_min_mean_vec in form of 1*1
  auto **y_vec_min_mean_vec = new EncodedNumber *[1];
  y_vec_min_mean_vec[0] = new EncodedNumber[1];

  // calculate [y_i] - [mean_y_cipher] in form of N*1
  auto **y_vec_min_mean_y_cipher = new EncodedNumber *[num_instance];
  for (int i = 0; i < num_instance; i++) {
    y_vec_min_mean_y_cipher[i] = new EncodedNumber[1];
  }
  // assign value
  for (int i = 0; i < num_instance; i++) {
    EncodedNumber tmp_res;
    djcs_t_aux_ee_add_ext(phe_pub_key, tmp_res, mean_y_cipher_neg, predictions[i]);
    y_vec_min_mean_y_cipher[i][0] = tmp_res;
  }

  int y_min_mean_y_cipher_pre = std::abs(y_vec_min_mean_y_cipher[0][0].getter_exponent());
  log_info("[pearson_fl]: 6 calculate [y] - [mean_y], result's precision ="
               + std::to_string(y_min_mean_y_cipher_pre));

  log_info("[pearson_fl]: 6. calculate the (<w> * {[y] - [mean_y]}) ** 2");

  // 6. calculate q2
  // make e_share to two dim,
  two_d_e_share_vec.push_back(e_share_vec);
  cipher_shares_mat_mul(party,
                        two_d_e_share_vec, // 1*N
                        y_vec_min_mean_y_cipher, // N * 1
                        1, num_instance, num_instance, 1,
                        y_vec_min_mean_vec);

  log_info("[pearson_fl]: 7.1 calculate q2 done");

  // convert q2 into share
  ciphers_to_secret_shares(party, y_vec_min_mean_vec[0],
                           q2_shares,
                           1,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);
  log_info("[pearson_fl]: 7.2 convert q2 cipher to share");

  log_info(
      "[pearson_fl]: 8. parties begin to calculate WPCC for features" + std::to_string(party.getter_feature_num()));

  // 1. calculate local feature mean
  for (int feature_id = 0; feature_id < party.getter_feature_num(); feature_id++) {
    // process one feature each time, feature_vector_plains: 1*N
    auto **feature_vector_plains = new EncodedNumber *[1];
    feature_vector_plains[0] = new EncodedNumber[num_instance];
    for (int sample_id = 0; sample_id < num_instance; sample_id++) {
      // assign value
      feature_vector_plains[0][sample_id].set_double(phe_pub_key->n[0],
                                                     train_data[sample_id][feature_id],
                                                     PHE_FIXED_POINT_PRECISION);
    }
    // calculate F*[W], 1*1
    auto *feature_multiply_w_cipher = new EncodedNumber[1];
    djcs_t_aux_vec_mat_ep_mult(phe_pub_key,
                               party.phe_random,
                               feature_multiply_w_cipher,
                               sss_weight_cipher,
                               feature_vector_plains,
                               1,
                               num_instance);

    log_info("[pearson_fl]: 9.1. calculate F*[W]");

    // convert it to share for further <tmp> / <sum_w>
    std::vector<double> tmp_shares;
    ciphers_to_secret_shares(party, feature_multiply_w_cipher,
                             tmp_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);
    log_info("[pearson_fl]: 9.3. convert it to share ");

    // calculate mean of mu
    std::vector<double> private_values_for_feature;
    private_values_for_feature.push_back(tmp_shares[0]);
    private_values_for_feature.push_back(sum_sss_weight_share[0]);
    std::vector<double> mean_f_share;
    comp_type = falcon::PEARSON_Division;
    std::promise<std::vector<double>> promise_values_mean_f;
    std::future<std::vector<double>> future_values_mean_f = promise_values_mean_f.get_future();
    std::thread spdz_dist_mean_f(spdz_lime_computation,
                                 party.party_num,
                                 party.party_id,
                                 party.executor_mpc_ports,
                                 party.host_names,
                                 0,
                                 public_values, // no public value needed
                                 private_values_for_feature.size(),  // 2
                                 private_values_for_feature, // <mean_F>, <sum_w>
                                 comp_type,
                                 &promise_values_mean_f);
    std::vector<double> res_mean_f = future_values_mean_f.get();
    spdz_dist_mean_f.join();
    mean_f_share = res_mean_f;

    log_info("[pearson_fl]: 9.4. computes <tmp> / <sum_w>");

    // convert mu back to cipher
    auto *mean_f_cipher = new EncodedNumber[1];
    secret_shares_to_ciphers(party,
                             mean_f_cipher,
                             mean_f_share,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    log_info("[pearson_fl]: 9.5. convert mu back to cipher");

    // calculate squared mean value f
    std::vector<double> squared_mean_f_share;
    squared_mean_f_share.push_back(mean_f_share[0] * mean_f_share[0]);

    // convert squared_mean_f back to cipher
    auto *squared_mean_f_cipher = new EncodedNumber[1];
    secret_shares_to_ciphers(party,
                             squared_mean_f_cipher,
                             squared_mean_f_share,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    log_info("[pearson_fl]: 9.6. convert squared_mean_f back to cipher and convert to cipher");

    // calculate numerator in equ 11
    // encrypt the feature vector first
    auto *encrypted_feature = new EncodedNumber[num_instance];
    for (int i = 0; i < num_instance; i++) {
      encrypted_feature[i].set_double(phe_pub_key->n[0], train_data[i][feature_id], PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_feature[i], encrypted_feature[i]);
    }

    // calculate [f_i] - [mean_f_cipher], in form of
    EncodedNumber mean_f_cipher_neg;
    djcs_t_aux_ep_mul(phe_pub_key, mean_f_cipher_neg, squared_mean_f_cipher[0], neg_one_int);

    // N*1
    auto **f_vec_min_mean_f_cipher = new EncodedNumber *[num_instance];
    for (int index = 0; index < num_instance; index++) {
      EncodedNumber f_min_mean_f_cipher;
      djcs_t_aux_ee_add(phe_pub_key, f_min_mean_f_cipher, mean_f_cipher_neg, encrypted_feature[index]);
      f_vec_min_mean_f_cipher[index] = new EncodedNumber[1];
      f_vec_min_mean_f_cipher[index][0] = f_min_mean_f_cipher;
    }

    log_info("[pearson_fl]: 9.7. calculate [f_i] - [mean_f_cipher],");

    // calculate <w> * ([F] - [mean_F]), 1*1
    auto **f_vec_min_mean_vec_share = new EncodedNumber *[1];
    f_vec_min_mean_vec_share[0] = new EncodedNumber[1];
    cipher_shares_mat_mul(party,
                          two_d_e_share_vec, // 1*N
                          f_vec_min_mean_f_cipher, // N*1
                          1, num_instance, num_instance, 1,
                          f_vec_min_mean_vec_share);

    log_info("[pearson_fl]: 9.8. calculate <w> * ([F] - [mean_F]) ");

    // get p share, 1*1
    std::vector<double> p_shares;
    ciphers_to_secret_shares(party, f_vec_min_mean_vec_share[0],
                             p_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    log_info("[pearson_fl]: 9.9. convert <w> * ([F] - [mean_F]) to shares ");

    // calculate (f-mean_f) ** 2, N*1
    auto **tmp_vec_cipher = new EncodedNumber *[num_instance];
    for (int i = 0; i < num_instance; i++) {
      tmp_vec_cipher[i] = new EncodedNumber[1];
    }

    for (int i = 0; i < num_instance; i++) {
      // calculate [f**2]
      EncodedNumber squared_feature_value_cipher;
      squared_feature_value_cipher.set_double(phe_pub_key->n[0],
                                              train_data[i][feature_id] * train_data[i][feature_id],
                                              PHE_FIXED_POINT_PRECISION * PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, squared_feature_value_cipher, squared_feature_value_cipher);
      log_info("[pearson_fl]: 9.10. for each instance, calculate [f**2] ");

      // calculate -2f*[mean_f]`
      EncodedNumber middle_value;
      EncodedNumber neg_2f;
      double value = -2 * train_data[i][feature_id];
      neg_2f.set_integer(phe_pub_key->n[0], value);
      djcs_t_aux_ep_mul(phe_pub_key, middle_value, mean_f_cipher[0], neg_2f);

      log_info("[pearson_fl]: 9.11. for each instance,  calculate -2f*[mean_f]");

      // calculate [f**2] + -2f*[mean_f] + [mean_f**2]
      EncodedNumber tmp_vec_ele;
      djcs_t_aux_ee_add_ext(phe_pub_key, tmp_vec_ele, squared_feature_value_cipher, middle_value);
      djcs_t_aux_ee_add_ext(phe_pub_key, tmp_vec_ele, tmp_vec_ele, squared_mean_f_cipher[0]);
      log_info("[pearson_fl]: 9.12. for each instance, calculate [f**2] + -2f*[mean_f] + [mean_f**2]");

      tmp_vec_cipher[i][0] = tmp_vec_ele;
    }

    log_info("[pearson_fl]: 9.13. (f-mean_f) ** 2, ");

    // calculate q1 1*1
    auto **q1_cipher = new EncodedNumber *[1];
    q1_cipher[0] = new EncodedNumber[1];

    cipher_shares_mat_mul(party,
                          two_d_sss_weights_share, // 1*N
                          tmp_vec_cipher, // N*1
                          1, num_instance, num_instance, 1,
                          q1_cipher);

    log_info("[pearson_fl]: 9.14. calculate q1 ");

    // convert cipher to shares
    std::vector<double> q1_shares;
    ciphers_to_secret_shares(party, q1_cipher[0],
                             q1_shares,
                             1,
                             ACTIVE_PARTY_ID,
                             PHE_FIXED_POINT_PRECISION);

    // for this feature, calculate WPCC share, wpcc = <p> / (<q1> * <q2>)
    std::vector<double> private_values_wpcc;
    private_values_wpcc.push_back(p_shares[0]);
    private_values_wpcc.push_back(q1_shares[0]);
    private_values_wpcc.push_back(q2_shares[0]);
    std::vector<double> wpcc_shares;
    falcon::SpdzLimeCompType comp_type_wpcc = falcon::PEARSON_Div_with_SquareRoot;
    std::promise<std::vector<double>> promise_values_wpcc;
    std::future<std::vector<double>> future_values_wpcc = promise_values_wpcc.get_future();
    std::thread spdz_dist_wpcc(spdz_lime_computation,
                               party.party_num,
                               party.party_id,
                               party.executor_mpc_ports,
                               party.host_names,
                               0,
                               public_values, // no public value needed
                               private_values_wpcc.size(), // 3
                               private_values_wpcc, // <mean_y>, <sum_w>
                               comp_type_wpcc,
                               &promise_values_wpcc);
    std::vector<double> res_wpcc = future_values_wpcc.get();
    spdz_dist_wpcc.join();
    wpcc_shares = res_wpcc;
    wpcc_vec.push_back(wpcc_shares[0]);

    log_info("[pearson_fl]: 9.15. calculate WPCC for feature " + std::to_string(feature_id));

    delete[] feature_vector_plains[0];
    delete[] feature_vector_plains;
    for (int i = 0; i< num_instance; i++){
      delete[] f_vec_min_mean_f_cipher[i];
    }
    delete[] f_vec_min_mean_f_cipher;
    delete[] f_vec_min_mean_vec_share[0];
    delete[] f_vec_min_mean_vec_share;
    delete[] feature_multiply_w_cipher;
    delete[] mean_f_cipher;
    delete[] squared_mean_f_cipher;
    delete[] encrypted_feature;
    for (int i = 0; i< num_instance; i++){
      delete[] tmp_vec_cipher[i];
    }
    delete[] tmp_vec_cipher;
    delete[] q1_cipher[0];
    delete[] q1_cipher;
  }

  djcs_t_free_public_key(phe_pub_key);

  log_info("[pearson_fl]: 9.16. All done, begin to clear the memory");

  // clean the code
  for (int i = 0; i < num_instance; i++) {
    delete[] two_d_prediction_cipher[i];
  }
  delete[] two_d_prediction_cipher;
  for (int i = 0; i < num_instance; i++) {
    delete[] y_vec_min_mean_y_cipher[i];
  }
  delete[] y_vec_min_mean_y_cipher;
  delete[] sss_weight_cipher;
  delete[] two_d_sss_weight_mul_pred_cipher[0];
  delete[] two_d_sss_weight_mul_pred_cipher;
  delete[] mean_y_cipher;
  delete[] y_vec_min_mean_vec[0];
  delete[] y_vec_min_mean_vec;

  log_info("[pearson_fl]: 9.17. Clear the memory done, return wpcc shares for all features");
  return wpcc_vec;
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
  std::cout << "begin connect to spdz parties" << std::endl;
  std::cout << "party_num = " << party_num << std::endl;
  for (int i = 0; i < party_num; i++) {
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
  LOG(INFO) << "Finish setup socket connections to spdz engines.";
  std::cout << "Finish setup socket connections to spdz engines." << std::endl;
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
  LOG(INFO) << "Finish initializing gfp field.";
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;
  google::FlushLogFiles(google::INFO);

  // send data to spdz parties
  std::cout << "party_id = " << party_id << std::endl;
  LOG(INFO) << "party_id = " << party_id;
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(lime_comp_type);
    std::cout << "lime_comp_type = " << lime_comp_type << std::endl;
    LOG(INFO) << "lime_comp_type = " << lime_comp_type;
    google::FlushLogFiles(google::INFO);
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends public values to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  LOG(INFO) << "active party sending all public value to all mpc  = " << lime_comp_type;
  google::FlushLogFiles(google::INFO);

  // all the parties send private shares
  std::cout << "private value size = " << private_value_size << std::endl;
  LOG(INFO) << "private value size = " << private_value_size;
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
      LOG(INFO) << "SPDZ calculate mean value <p> /( <q1> * <q2>) ";
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, private_value_size);
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

