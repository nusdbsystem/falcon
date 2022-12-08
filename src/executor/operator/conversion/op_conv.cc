//
// Created by root on 4/21/22.
//

#include <falcon/operator/conversion/op_conv.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/base64.h>
#include <omp.h>

void collaborative_decrypt(const Party &party, EncodedNumber *src_ciphers,
                           EncodedNumber *dest_plains, int size, int req_party_id) {

//  log_info("[collaborative_decrypt] size = " + std::to_string(size));
//  const clock_t co_dec_start_time = clock();

  // retrieve phe pub key and auth server
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  djcs_t_auth_server *phe_auth_server = djcs_t_init_auth_server();
  party.getter_phe_pub_key(phe_pub_key);
  party.getter_phe_auth_server(phe_auth_server);
  // partially decrypt the ciphertext vector
  auto *partial_decryption = new EncodedNumber[size];
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    djcs_t_aux_partial_decrypt(phe_pub_key, phe_auth_server,
                               partial_decryption[i], src_ciphers[i]);
  }
//  const clock_t local_partial_dec_time = clock();
//  double local_dec_consumed_time =
//      double(local_partial_dec_time - co_dec_start_time) / CLOCKS_PER_SEC;
//  log_info("Collaborative local decryption time = " + std::to_string(local_dec_consumed_time));

  if (party.party_id == req_party_id) {
//
//    const clock_t agg_dec_shares_start_time = clock();

    // create 2D-array m*n to store the decryption shares,
    // m = size, n = party_num, such that each row i represents
    // all the shares for the i-th ciphertext
    auto **decryption_shares = new EncodedNumber *[size];
    for (int i = 0; i < size; i++) {
      decryption_shares[i] = new EncodedNumber[party.party_num];
    }

    // collect all the decryption shares
    for (int id = 0; id < party.party_num; id++) {
      if (id == party.party_id) {
        // copy self partially decrypted shares
        for (int i = 0; i < size; i++) {
          decryption_shares[i][id] = partial_decryption[i];
        }
      } else {
        std::string recv_partial_decryption_str;
        party.recv_long_message(id, recv_partial_decryption_str);
        auto *recv_partial_decryption = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_partial_decryption, size,
                                         recv_partial_decryption_str);
        // copy other party's decrypted shares
        for (int i = 0; i < size; i++) {
          decryption_shares[i][id] = recv_partial_decryption[i];
        }
        delete[] recv_partial_decryption;
      }
    }

    // share combine for decryption
    omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
    for (int i = 0; i < size; i++) {
      djcs_t_aux_share_combine(phe_pub_key, dest_plains[i],
                               decryption_shares[i], party.party_num);
    }

    // free memory
    for (int i = 0; i < size; i++) {
      delete[] decryption_shares[i];
    }
    delete[] decryption_shares;
//
//    const clock_t agg_dec_shares_end_time = clock();
//    double active_party_agg_shares_time =
//        double(agg_dec_shares_end_time - agg_dec_shares_start_time) / CLOCKS_PER_SEC;
//    log_info("Active party aggregate decryption shares time = " + std::to_string(active_party_agg_shares_time));

  } else {
    // send decrypted shares to the req_party_id
    std::string partial_decryption_str;
    serialize_encoded_number_array(partial_decryption, size,
                                   partial_decryption_str);
    party.send_long_message(req_party_id, partial_decryption_str);
  }

//  const clock_t co_dec_end_time = clock();
//  double co_dec_consumed_time =
//      double(co_dec_end_time - co_dec_start_time) / CLOCKS_PER_SEC;
//  log_info("Collaborative decryption time = " + std::to_string(co_dec_consumed_time));

  delete[] partial_decryption;
  djcs_t_free_public_key(phe_pub_key);
  djcs_t_free_auth_server(phe_auth_server);
}

void ciphers_to_secret_shares(const Party &party, EncodedNumber *src_ciphers,
                              std::vector<double> &secret_shares,
                              int size, int req_party_id, int phe_precision) {
  // retrieve phe pub key and auth server
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // each party generates a random vector with size values
  // (the request party will add the summation to the share)
  // ui randomly chooses ri belongs to Zq and encrypts it as [ri]
  auto *encrypted_shares = new EncodedNumber[size];
  secret_shares = std::vector<double>(size, 0.0);
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    // TODO: check how to replace with spdz random values
    if (phe_precision != 0) {
      auto s = static_cast<double>(rand() % MAXIMUM_RAND_VALUE);
      encrypted_shares[i].set_double(phe_pub_key->n[0], s, phe_precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_shares[i],
                         encrypted_shares[i]);
      secret_shares[i] = 0 - s;
    } else {
      int s = rand() % MAXIMUM_RAND_VALUE;
      encrypted_shares[i].set_integer(phe_pub_key->n[0], s);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_shares[i],
                         encrypted_shares[i]);
      secret_shares[i] = 0 - s;
    }
  }

  // request party aggregate the shares and invoke collaborative decryption
  auto *aggregated_shares = new EncodedNumber[size];
  if (party.party_id == req_party_id) {
    omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
    for (int i = 0; i < size; i++) {
      aggregated_shares[i] = encrypted_shares[i];
      djcs_t_aux_ee_add_ext(phe_pub_key, aggregated_shares[i], aggregated_shares[i],
                            src_ciphers[i]);
    }
    // recv message and add to aggregated shares,
    // u1 computes [e] = [x]+[r1]+..+[rm]
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_encrypted_shares_str;
        party.recv_long_message(id, recv_encrypted_shares_str);
        auto *recv_encrypted_shares = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_encrypted_shares, size,
                                         recv_encrypted_shares_str);
        omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add_ext(phe_pub_key, aggregated_shares[i],
                                aggregated_shares[i], recv_encrypted_shares[i]);
        }
        delete[] recv_encrypted_shares;
      }
    }
    // serialize and send to other parties
    std::string aggregated_shares_str;
    serialize_encoded_number_array(aggregated_shares, size,
                                   aggregated_shares_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, aggregated_shares_str);
      }
    }
  } else {
    // send encrypted shares to the request party
    // ui sends [ri] to u1
    std::string encrypted_shares_str;
    serialize_encoded_number_array(encrypted_shares, size,
                                   encrypted_shares_str);
    party.send_long_message(req_party_id, encrypted_shares_str);

    // receive aggregated shares from the request party
    std::string recv_aggregated_shares_str;
    party.recv_long_message(req_party_id, recv_aggregated_shares_str);
    deserialize_encoded_number_array(aggregated_shares, size,
                                     recv_aggregated_shares_str);
  }
  // collaborative decrypt the aggregated shares, clients jointly decrypt [e]
  auto *decrypted_sum = new EncodedNumber[size];
  collaborative_decrypt(party, aggregated_shares, decrypted_sum, size, req_party_id);
  // if request party, add the decoded results to the secret shares
  // u1 sets [x]1 = e − r1 mod q
  if (party.party_id == req_party_id) {
    for (int i = 0; i < size; i++) {
      if (phe_precision != 0) {
        double decoded_sum_i;
        decrypted_sum[i].decode(decoded_sum_i);
        secret_shares[i] += decoded_sum_i;
      } else {
        long decoded_sum_i;
        decrypted_sum[i].decode(decoded_sum_i);
        secret_shares[i] += (double) decoded_sum_i;
      }
    }
  }

  delete[] encrypted_shares;
  delete[] aggregated_shares;
  delete[] decrypted_sum;
  djcs_t_free_public_key(phe_pub_key);
}

void ciphers_mat_to_secret_shares_mat(const Party &party, EncodedNumber **src_ciphers_mat,
                                      std::vector<std::vector<double>> &secret_shares_mat,
                                      int row_size, int column_size,
                                      int req_party_id, int phe_precision) {
  // convert the ciphertext vector by vector
  for (int i = 0; i < row_size; i++) {
    std::vector<double> secret_shares_vec;
    ciphers_to_secret_shares(party, src_ciphers_mat[i],
                             secret_shares_vec, column_size,
                             req_party_id, phe_precision);
    secret_shares_mat.push_back(secret_shares_vec);
  }
}

void secret_shares_to_ciphers(const Party &party, EncodedNumber *dest_ciphers,
                              std::vector<double> secret_shares,
                              int size, int req_party_id, int phe_precision) {
  // retrieve phe pub key and auth server
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // encode and encrypt the secret shares
  // and send to req_party, who aggregates
  // and send back to the other parties
  auto *encrypted_shares = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    encrypted_shares[i].set_double(phe_pub_key->n[0], secret_shares[i],
                                   phe_precision);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_shares[i],
                       encrypted_shares[i]);
  }

  if (party.party_id == req_party_id) {
    // copy local encrypted_shares to dest_ciphers
    for (int i = 0; i < size; i++) {
      dest_ciphers[i] = encrypted_shares[i];
    }

    // receive from other parties and aggregate encrypted shares
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_encrypted_shares_str;
        party.recv_long_message(id, recv_encrypted_shares_str);
        auto *recv_encrypted_shares = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_encrypted_shares, size,
                                         recv_encrypted_shares_str);
        // homomorphic aggregation
        omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add_ext(phe_pub_key, dest_ciphers[i], dest_ciphers[i],
                                recv_encrypted_shares[i]);
        }
        delete[] recv_encrypted_shares;
      }
    }

    // serialize dest_ciphers and broadcast
    std::string dest_ciphers_str;
    serialize_encoded_number_array(dest_ciphers, size, dest_ciphers_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, dest_ciphers_str);
      }
    }
  } else {
    // serialize and send to req_party
    std::string encrypted_shares_str;
    serialize_encoded_number_array(encrypted_shares, size,
                                   encrypted_shares_str);
    party.send_long_message(req_party_id, encrypted_shares_str);

    // receive and set dest_ciphers
    std::string recv_dest_ciphers_str;
    party.recv_long_message(req_party_id, recv_dest_ciphers_str);
    deserialize_encoded_number_array(dest_ciphers, size, recv_dest_ciphers_str);
  }

  delete[] encrypted_shares;
  djcs_t_free_public_key(phe_pub_key);
}

/***********************************************************/
/***************** cipher & share operations ***************/
/***********************************************************/

void truncate_ciphers_precision(const Party &party, EncodedNumber *ciphers, int size,
                                int req_party_id, int dest_precision) {
  // check if the cipher precision is higher than the dest_precision
  // otherwise, no need to truncate the precision
  int src_precision = abs(ciphers[0].getter_exponent());
  if (src_precision == dest_precision) {
    log_info("src precision is the same as dest_precision, do nothing");
    return;
  }
  if (src_precision < dest_precision) {
    log_error("cannot increase ciphertext precision");
    exit(EXIT_FAILURE);
  }
  // step 1. broadcast the ciphers from req_party_id
  // broadcast_encoded_number_array(ciphers, size, req_party_id);
  // step 2. convert the ciphers into secret shares given src_precision
  std::vector<double> ciphers_shares;
  ciphers_to_secret_shares(party, ciphers, ciphers_shares, size, req_party_id, src_precision);
  // step 3. convert the secret shares into ciphers given dest_precision
  auto *dest_ciphers = new EncodedNumber[size];
  secret_shares_to_ciphers(party, dest_ciphers, ciphers_shares, size, req_party_id, dest_precision);
  // step 4. inplace ciphers by dest_ciphers
  for (int i = 0; i < size; i++) {
    ciphers[i] = dest_ciphers[i];
  }
  log_info("truncate the ciphers precision finished");
  delete[] dest_ciphers;
}

void ciphers_multi(const Party &party, EncodedNumber *res, EncodedNumber *ciphers1,
                   EncodedNumber *ciphers2, int size, int req_party_id) {
  // retrieve phe pub key and auth server
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // step 1: convert ciphers 1 to secret shares
  int cipher1_precision = std::abs(ciphers1[0].getter_exponent());
  int cipher2_precision = std::abs(ciphers2[0].getter_exponent());
  std::vector<double> ciphers1_shares;
  ciphers_to_secret_shares(party, ciphers1, ciphers1_shares,
                           size, req_party_id, cipher1_precision);
  auto *encoded_ciphers1_shares = new EncodedNumber[size];

  // step 2: aggregate the plaintext and ciphers2 multiplication
  auto *global_aggregation = new EncodedNumber[size];
  auto *local_aggregation = new EncodedNumber[size];
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    encoded_ciphers1_shares[i].set_double(phe_pub_key->n[0],
                                          ciphers1_shares[i]);
    djcs_t_aux_ep_mul(phe_pub_key,
                      local_aggregation[i],
                      ciphers2[i],
                      encoded_ciphers1_shares[i]);
  }
  if (party.party_id == req_party_id) {
    for (int i = 0; i < size; i++) {
      global_aggregation[i] = local_aggregation[i];
    }
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        auto *recv_local_aggregation = new EncodedNumber[size];
        std::string recv_local_aggregation_str;
        party.recv_long_message(id, recv_local_aggregation_str);
        deserialize_encoded_number_array(recv_local_aggregation,
                                         size, recv_local_aggregation_str);
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, global_aggregation[i],
                            global_aggregation[i], recv_local_aggregation[i]);
        }
        delete[] recv_local_aggregation;
      }
    }
  } else {
    // serialize and send to req_party_id
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_aggregation, size, local_aggregation_str);
    party.send_long_message(req_party_id, local_aggregation_str);
  }
  broadcast_encoded_number_array(party, global_aggregation, size, req_party_id);

  // step 3: write to result vector
  for (int i = 0; i < size; i++) {
    res[i] = global_aggregation[i];
  }

  delete[] encoded_ciphers1_shares;
  delete[] global_aggregation;
  delete[] local_aggregation;
  djcs_t_free_public_key(phe_pub_key);
}

void cipher_shares_mat_mul(const Party &party,
                           const std::vector<std::vector<double>> &shares,
                           EncodedNumber **ciphers,
                           int n_shares_row,
                           int n_shares_col,
                           int n_ciphers_row,
                           int n_ciphers_col,
                           EncodedNumber **ret) {
  // retrieve phe pub key and auth server
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // sanity check
  if (n_shares_row != shares.size() || n_shares_col != shares[0].size()) {
    log_error("[cipher_shares_mat_mul] the shares sizes do not match");
    exit(EXIT_FAILURE);
  }
  // check if the column size of shares = the row size of ciphers for matrix multiplication
  if (n_shares_col != n_ciphers_row) {
    log_error("[cipher_shares_mat_mul] matrix multiplication dimensions do not match");
    exit(EXIT_FAILURE);
  }

  // note that the shares and ciphers multiplication equals:
  // each party computes the mat_mul(shares, ciphers), and then aggregate the results
  auto **encoded_shares = new EncodedNumber *[n_shares_row];
  for (int i = 0; i < n_shares_row; i++) {
    encoded_shares[i] = new EncodedNumber[n_shares_col];
  }
  for (int i = 0; i < n_shares_row; i++) {
    for (int j = 0; j < n_shares_col; j++) {
      //      log_info("[cipher_shares_mat_mul] shares["
      //                   + std::to_string(i) + "][" + std::to_string(j) + "] = "
      //                   + std::to_string(shares[i][j]));
      encoded_shares[i][j].set_double(phe_pub_key->n[0], shares[i][j], PHE_FIXED_POINT_PRECISION);
    }
  }

  auto **local_mul_res = new EncodedNumber *[n_shares_row];
  for (int i = 0; i < n_shares_row; i++) {
    local_mul_res[i] = new EncodedNumber[n_ciphers_col];
  }

  //  log_info("[cipher_shares_mat_mul] debug ciphers matrix inside cipher_shares_mat_mul");
  //  display_encrypted_matrix(party, n_ciphers_row, n_ciphers_col, ciphers);

  djcs_t_aux_mat_mat_ep_mult(phe_pub_key,
                             party.phe_random,
                             local_mul_res,
                             ciphers,
                             encoded_shares,
                             n_ciphers_row,
                             n_ciphers_col,
                             n_shares_row,
                             n_shares_col);

  //  broadcast_encoded_number_matrix(party, local_mul_res, n_shares_row, n_ciphers_col, 0);
  //  log_info("[cipher_shares_mat_mul] display local_mul_res for debug ");
  //  display_encrypted_matrix(party, n_shares_row, n_ciphers_col, local_mul_res);

  //  log_info("[cipher_shares_mat_mul] local_mul_res.ret = " + std::to_string(std::abs(local_mul_res[0][0].getter_exponent())));

  if (party.party_type == falcon::ACTIVE_PARTY) {
    // assign ret = local_mul_res, and receive from passive parties and aggregate
    for (int i = 0; i < n_shares_row; i++) {
      for (int j = 0; j < n_ciphers_col; j++) {
        ret[i][j] = local_mul_res[i][j];
      }
    }
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_id_local_mul_res_str;
        party.recv_long_message(id, recv_id_local_mul_res_str);
        //        std::string b64_recv_id_local_mul_res_str = base64_encode(reinterpret_cast<const BYTE *>(
        //            recv_id_local_mul_res_str.c_str()), recv_id_local_mul_res_str.size());
        //        log_info("[cipher_shares_mat_mul] receive recv_id_local_mul_res_str: " + b64_recv_id_local_mul_res_str);
        auto **recv_local_mul_res = new EncodedNumber *[n_shares_row];
        for (int i = 0; i < n_shares_row; i++) {
          recv_local_mul_res[i] = new EncodedNumber[n_ciphers_col];
        }
        deserialize_encoded_number_matrix(recv_local_mul_res,
                                          n_shares_row,
                                          n_ciphers_col,
                                          recv_id_local_mul_res_str);
        // aggregate
        djcs_t_aux_matrix_ele_wise_ee_add(phe_pub_key,
                                          ret,
                                          ret,
                                          recv_local_mul_res,
                                          n_shares_row,
                                          n_ciphers_col);

        for (int i = 0; i < n_shares_row; i++) {
          delete[] recv_local_mul_res[i];
        }
        delete[] recv_local_mul_res;
      }
    }
  } else {
    std::string local_mul_res_str;
    serialize_encoded_number_matrix(local_mul_res, n_shares_row, n_ciphers_col, local_mul_res_str);
    //    std::string b64_local_mul_res_str = base64_encode(reinterpret_cast<const BYTE *>(
    //                                                        local_mul_res_str.c_str()),
    //                                                    local_mul_res_str.size());
    //    log_info("[cipher_shares_mat_mul] sent b64_local_mul_res_str: " + b64_local_mul_res_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_mul_res_str);
  }
  broadcast_encoded_number_matrix(party, ret, n_shares_row, n_ciphers_col, ACTIVE_PARTY_ID);

  //  log_info("[cipher_shares_mat_mul] debug ret after aggregation");
  //  display_encrypted_matrix(party, n_shares_row, n_ciphers_col, ret);

  for (int i = 0; i < n_shares_row; i++) {
    delete[] encoded_shares[i];
    delete[] local_mul_res[i];
  }
  delete[] encoded_shares;
  delete[] local_mul_res;
  djcs_t_free_public_key(phe_pub_key);
}

void transpose_encoded_mat(EncodedNumber **source_mat,
                           int n_source_row,
                           int n_source_col,
                           EncodedNumber **ret_mat) {
  if ((n_source_row == 0) || (n_source_col == 0)) {
    log_error("[transpose_encoded_mat] matrix is empty");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < n_source_row; i++) {
    for (int j = 0; j < n_source_col; j++) {
      ret_mat[j][i] = source_mat[i][j];
    }
  }
}

void cipher_shares_ele_wise_vec_mul(const Party &party,
                                    const std::vector<double> &shares,
                                    EncodedNumber *ciphers,
                                    int size,
                                    EncodedNumber *ret) {
  if (size != shares.size()) {
    log_error("[cipher_shares_ele_wise_vec_mul] size does not match");
    exit(EXIT_FAILURE);
  }
  // retrieve phe pub key and auth server
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  auto *local_agg = new EncodedNumber[size];
  auto *plain_shares = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    plain_shares[i].set_double(phe_pub_key->n[0], shares[i]);
  }
  djcs_t_aux_vec_ele_wise_ep_mul(phe_pub_key, local_agg, ciphers, plain_shares, size);

  if (party.party_id == falcon::ACTIVE_PARTY) {
    // copy local agg
    for (int i = 0; i < size; i++) {
      ret[i] = local_agg[i];
    }
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_local_agg_str;
        party.recv_long_message(id, recv_local_agg_str);
        auto *recv_local_agg = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_local_agg, size, recv_local_agg_str);
        djcs_t_aux_vec_ele_wise_ee_add_ext(phe_pub_key, ret, ret, recv_local_agg, size);
        delete[] recv_local_agg;
      }
    }
  } else {
    std::string local_agg_str;
    serialize_encoded_number_array(local_agg, size, local_agg_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_agg_str);
  }

  broadcast_encoded_number_array(party, ret, size, ACTIVE_PARTY_ID);

  delete[] local_agg;
  delete[] plain_shares;
  djcs_t_free_public_key(phe_pub_key);
}

std::vector<double> display_shares_vector(
    const Party &party,
    const std::vector<double> &vec) {
  log_info("------------------ [display_shares_vector] ---------------");
  // display secret share vector
  std::vector<double> agg(vec.size(), 0.0);
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < vec.size(); i++) {
      agg[i] = vec[i];
    }
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_vec_str;
        std::vector<double> recv_vec;
        party.recv_long_message(id, recv_vec_str);
        deserialize_double_array(recv_vec, recv_vec_str);
        for (int i = 0; i < recv_vec.size(); i++) {
          agg[i] += recv_vec[i];
        }
      }
    }
    for (int i = 0; i < agg.size(); i++) {
      log_info("[display_shares_vector] vector[" + std::to_string(i) + "] = " + std::to_string(agg[i]));
    }
  } else {
    std::string send_vec_str;
    serialize_double_array(vec, send_vec_str);
    party.send_long_message(ACTIVE_PARTY_ID, send_vec_str);
  }
  return agg;
}

std::vector<std::vector<double>> display_shares_matrix(
    const Party &party,
    const std::vector<std::vector<double>> &mat) {
  log_info("------------------ [display_shares_matrix] ---------------");
  // display secret share matrix
  std::vector<std::vector<double>> agg_mat(mat.size(), std::vector<double>(mat[0].size(), 0.0));
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < mat.size(); i++) {
      for (int j = 0; j < mat[0].size(); j++) {
        agg_mat[i][j] = mat[i][j];
      }
    }
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_mat_str;
        std::vector<std::vector<double>> recv_mat;
        party.recv_long_message(id, recv_mat_str);
        deserialize_double_matrix(recv_mat, recv_mat_str);
        for (int i = 0; i < recv_mat.size(); i++) {
          for (int j = 0; j < recv_mat[0].size(); j++) {
            agg_mat[i][j] += recv_mat[i][j];
          }
        }
      }
    }
    for (int i = 0; i < agg_mat.size(); i++) {
      for (int j = 0; j < agg_mat[0].size(); j++) {
        log_info("[display_shares_matrix] matrix["
        + std::to_string(i) + "][" + std::to_string(j) + "] = " + std::to_string(agg_mat[i][j]));
      }
    }
  } else {
    std::string send_mat_str;
    serialize_double_matrix(mat, send_mat_str);
    party.send_long_message(ACTIVE_PARTY_ID, send_mat_str);
  }

  return agg_mat;
}
