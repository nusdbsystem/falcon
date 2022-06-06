//
// Created by root on 4/21/22.
//

#include <falcon/operator/conversion/op_conv.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/party/info_exchange.h>

void collaborative_decrypt(const Party& party, EncodedNumber* src_ciphers,
                           EncodedNumber* dest_plains, int size, int req_party_id) {
  // retrieve phe pub key and auth server
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  djcs_t_auth_server* phe_auth_server = djcs_t_init_auth_server();
  party.getter_phe_pub_key(phe_pub_key);
  party.getter_phe_auth_server(phe_auth_server);
  // partially decrypt the ciphertext vector
  auto * partial_decryption = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    djcs_t_aux_partial_decrypt(phe_pub_key, phe_auth_server,
                               partial_decryption[i], src_ciphers[i]);
  }

  if (party.party_id == req_party_id) {
    // create 2D-array m*n to store the decryption shares,
    // m = size, n = party_num, such that each row i represents
    // all the shares for the i-th ciphertext
    auto** decryption_shares = new EncodedNumber*[size];
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
        auto* recv_partial_decryption = new EncodedNumber[size];
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
    for (int i = 0; i < size; i++) {
      djcs_t_aux_share_combine(phe_pub_key, dest_plains[i],
                               decryption_shares[i], party.party_num);
    }

    // free memory
    for (int i = 0; i < size; i++) {
      delete[] decryption_shares[i];
    }
    delete[] decryption_shares;
  } else {
    // send decrypted shares to the req_party_id
    std::string partial_decryption_str;
    serialize_encoded_number_array(partial_decryption, size,
                                   partial_decryption_str);
    party.send_long_message(req_party_id, partial_decryption_str);
  }

  delete[] partial_decryption;
  djcs_t_free_public_key(phe_pub_key);
  djcs_t_free_auth_server(phe_auth_server);
}

void ciphers_to_secret_shares(const Party& party, EncodedNumber* src_ciphers,
                              std::vector<double>& secret_shares,
                              int size, int req_party_id, int phe_precision) {
  // retrieve phe pub key and auth server
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // each party generates a random vector with size values
  // (the request party will add the summation to the share)
  // ui randomly chooses ri belongs to Zq and encrypts it as [ri]
  auto* encrypted_shares = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    // TODO: check how to replace with spdz random values
    if (phe_precision != 0) {
      auto s = static_cast<double>(rand() % MAXIMUM_RAND_VALUE);
      encrypted_shares[i].set_double(phe_pub_key->n[0], s, phe_precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_shares[i],
                         encrypted_shares[i]);
      secret_shares.push_back(0 - s);
    } else {
      int s = rand() % MAXIMUM_RAND_VALUE;
      encrypted_shares[i].set_integer(phe_pub_key->n[0], s);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_shares[i],
                         encrypted_shares[i]);
      secret_shares.push_back(0 - s);
    }
  }

  // request party aggregate the shares and invoke collaborative decryption
  auto* aggregated_shares = new EncodedNumber[size];
  if (party.party_id == req_party_id) {
    for (int i = 0; i < size; i++) {
      aggregated_shares[i] = encrypted_shares[i];
      djcs_t_aux_ee_add(phe_pub_key, aggregated_shares[i], aggregated_shares[i],
                        src_ciphers[i]);
    }
    // recv message and add to aggregated shares,
    // u1 computes [e] = [x]+[r1]+..+[rm]
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_encrypted_shares_str;
        party.recv_long_message(id, recv_encrypted_shares_str);
        auto* recv_encrypted_shares = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_encrypted_shares, size,
                                         recv_encrypted_shares_str);
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, aggregated_shares[i],
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
  auto* decrypted_sum = new EncodedNumber[size];
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
        secret_shares[i] += (double)decoded_sum_i;
      }
    }
  }

  delete[] encrypted_shares;
  delete[] aggregated_shares;
  delete[] decrypted_sum;
  djcs_t_free_public_key(phe_pub_key);
}

void ciphers_mat_to_secret_shares_mat(const Party& party, EncodedNumber** src_ciphers_mat,
                                      std::vector<std::vector<double>>& secret_shares_mat,
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

void secret_shares_to_ciphers(const Party& party, EncodedNumber* dest_ciphers,
                              std::vector<double> secret_shares,
                              int size, int req_party_id, int phe_precision) {
  // retrieve phe pub key and auth server
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // encode and encrypt the secret shares
  // and send to req_party, who aggregates
  // and send back to the other parties
  auto* encrypted_shares = new EncodedNumber[size];
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
        auto* recv_encrypted_shares = new EncodedNumber[size];
        deserialize_encoded_number_array(recv_encrypted_shares, size,
                                         recv_encrypted_shares_str);
        // homomorphic aggregation
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, dest_ciphers[i], dest_ciphers[i],
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

void truncate_ciphers_precision(const Party& party, EncodedNumber *ciphers, int size,
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
  ciphers_to_secret_shares(party, ciphers,ciphers_shares, size, req_party_id, src_precision);
  // step 3. convert the secret shares into ciphers given dest_precision
  auto *dest_ciphers = new EncodedNumber[size];
  secret_shares_to_ciphers(party, dest_ciphers,ciphers_shares, size, req_party_id, dest_precision);
  // step 4. inplace ciphers by dest_ciphers
  for (int i = 0; i < size; i++) {
    ciphers[i] = dest_ciphers[i];
  }
  log_info("truncate the ciphers precision finished");
  delete [] dest_ciphers;
}

void ciphers_multi(const Party& party, EncodedNumber *res, EncodedNumber *ciphers1,
                          EncodedNumber *ciphers2, int size, int req_party_id) {
  // retrieve phe pub key and auth server
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // step 1: convert ciphers 1 to secret shares
  int cipher1_precision = std::abs(ciphers1[0].getter_exponent());
  int cipher2_precision = std::abs(ciphers2[0].getter_exponent());
  std::vector<double> ciphers1_shares;
  ciphers_to_secret_shares(party, ciphers1, ciphers1_shares,
                           size, req_party_id, cipher1_precision);
  auto* encoded_ciphers1_shares = new EncodedNumber[size];

  // step 2: aggregate the plaintext and ciphers2 multiplication
  auto* global_aggregation = new EncodedNumber[size];
  auto* local_aggregation = new EncodedNumber[size];
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
        auto* recv_local_aggregation = new EncodedNumber[size];
        std::string recv_local_aggregation_str;
        party.recv_long_message(id, recv_local_aggregation_str);
        deserialize_encoded_number_array(recv_local_aggregation,
                                         size, recv_local_aggregation_str);
        for (int i = 0; i < size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, global_aggregation[i],
                            global_aggregation[i], recv_local_aggregation[i]);
        }
        delete [] recv_local_aggregation;
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

  delete [] encoded_ciphers1_shares;
  delete [] global_aggregation;
  delete [] local_aggregation;
  djcs_t_free_public_key(phe_pub_key);
}

void cipher_shares_mat_mul(const Party& party,
                           const std::vector<std::vector<double>>& shares,
                           EncodedNumber** ciphers,
                           int n_shares_row,
                           int n_shares_col,
                           int n_ciphers_row,
                           int n_ciphers_col,
                           EncodedNumber** ret,
                           int& ret_prec) {
  // retrieve phe pub key and auth server
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
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
  auto** encoded_shares = new EncodedNumber*[n_shares_row];
  for (int i = 0; i < n_shares_row; i++) {
    encoded_shares[i] = new EncodedNumber[n_shares_col];
  }
  auto** local_mul_res = new EncodedNumber*[n_shares_row];
  for (int i = 0; i < n_shares_row; i++) {
    local_mul_res[i] = new EncodedNumber[n_ciphers_col];
  }
  djcs_t_aux_mat_mat_ep_mult(phe_pub_key,
                             party.phe_random,
                             local_mul_res,
                             ciphers,
                             encoded_shares,
                             n_ciphers_row,
                             n_ciphers_col,
                             n_shares_row,
                             n_shares_col);

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
        deserialize_encoded_number_matrix(local_mul_res,
                                          n_shares_row,
                                          n_ciphers_col,
                                          recv_id_local_mul_res_str);
        // aggregate
        djcs_t_aux_matrix_ele_wise_ee_add(phe_pub_key,
                                          ret,
                                          ret,
                                          local_mul_res,
                                          n_shares_row,
                                          n_ciphers_col);
      }
    }
  } else {
    std::string local_mul_res_str;
    serialize_encoded_number_matrix(local_mul_res,
                                    n_shares_row,
                                    n_ciphers_col,
                                    local_mul_res_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_mul_res_str);
  }
  broadcast_encoded_number_matrix(party, ret, n_shares_row, n_ciphers_col, ACTIVE_PARTY_ID);

  ret_prec = std::abs(ret[0][0].getter_exponent());

  for (int i = 0; i < n_shares_row; i++) {
    delete [] encoded_shares[i];
    delete [] local_mul_res[i];
  }
  delete [] encoded_shares;
  delete [] local_mul_res;
  djcs_t_free_public_key(phe_pub_key);
}
