//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/vertical/nn/layer.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/operator/conversion/op_conv.h>
#include <random>

Layer::Layer() {
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_fit_bias = true;
}

Layer::Layer(int num_inputs, int num_outputs,
             bool with_bias, const std::string &activation_func_str) {
  m_num_inputs = num_inputs;
  m_num_outputs = num_outputs;
  m_fit_bias = with_bias;
  m_activation_func_str = activation_func_str;
  // init m_weight_mat
  m_weight_mat = new EncodedNumber*[m_num_inputs];
  for (int i = 0; i < m_num_inputs; i++) {
    m_weight_mat[i] = new EncodedNumber[m_num_outputs];
  }
  // init m_bias vector
  if (m_fit_bias) {
    m_bias = new EncodedNumber[m_num_outputs];
  }
}

Layer::Layer(const Layer &layer) {
  m_num_inputs = layer.m_num_inputs;
  m_num_outputs = layer.m_num_outputs;
  m_fit_bias = layer.m_fit_bias;
  m_activation_func_str = layer.m_activation_func_str;
  m_weight_mat = new EncodedNumber*[m_num_inputs];
  for (int i = 0; i < m_num_inputs; i++) {
    m_weight_mat[i] = new EncodedNumber[m_num_outputs];
  }
  m_bias = new EncodedNumber[m_num_outputs];
  for (int i = 0; i < m_num_inputs; i++) {
    for (int j = 0; j < m_num_outputs; j++) {
      m_weight_mat[i][j] = layer.m_weight_mat[i][j];
    }
  }
  for (int j = 0; j < m_num_outputs; j++) {
    m_bias[j] = layer.m_bias[j];
  }
}

Layer &Layer::operator=(const Layer &layer) {
  m_num_inputs = layer.m_num_inputs;
  m_num_outputs = layer.m_num_outputs;
  m_fit_bias = layer.m_fit_bias;
  m_activation_func_str = layer.m_activation_func_str;
  m_weight_mat = new EncodedNumber*[m_num_inputs];
  for (int i = 0; i < m_num_inputs; i++) {
    m_weight_mat[i] = new EncodedNumber[m_num_outputs];
  }
  m_bias = new EncodedNumber[m_num_outputs];
  for (int i = 0; i < m_num_inputs; i++) {
    for (int j = 0; j < m_num_outputs; j++) {
      m_weight_mat[i][j] = layer.m_weight_mat[i][j];
    }
  }
  for (int j = 0; j < m_num_outputs; j++) {
    m_bias[j] = layer.m_bias[j];
  }
}

Layer::~Layer() {
  for (int i = 0; i < m_num_inputs; i++) {
    delete [] m_weight_mat[i];
  }
  delete [] m_weight_mat;
  delete [] m_bias;
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_fit_bias = true;
}

void Layer::init_encrypted_weights(const Party &party, int precision) {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // according to sklearn init coef function, here
  // set factor=6.0 if the activation function is not 'logistic' or 'sigmoid'
  // otherwise set factor=2.0
  double factor = 6.0;
  if (m_activation_func_str == "sigmoid") {
    factor = 2.0;
  }
  // according to sklearn, limit = sqrt(factor / (num_inputs + num_outputs))
  double limit = sqrt(factor / (m_num_inputs + m_num_outputs));
  std::mt19937 mt(RANDOM_SEED);
  std::uniform_real_distribution<double> dist(-limit, limit);
  // generate random values for weights and bias
  std::vector<std::vector<double>> plain_rand_weight_mat;
  std::vector<double> plain_rand_bias_vec;
  for (int i = 0; i < m_num_inputs; i++) {
    std::vector<double> plain_rand_weight_vec;
    for (int j = 0; j < m_num_outputs; j++) {
      double r = dist(mt);
      plain_rand_weight_vec.push_back(r);
    }
    plain_rand_weight_mat.push_back(plain_rand_weight_vec);
  }
  for (int j = 0; j < m_num_outputs; j++) {
    double r = dist(mt);
    plain_rand_bias_vec.push_back(r);
  }

  // assign plain_rand_weight_mat and plain_rand_bias_vec to m_weight_mat and m_bias
  log_info("[Layer::init_encrypted_weights] m_num_inputs = " + std::to_string(m_num_inputs));
  log_info("[Layer::init_encrypted_weights] m_num_outputs = " + std::to_string(m_num_outputs));
  for (int i = 0; i < m_num_inputs; i++) {
    for (int j = 0; j < m_num_outputs; j++) {
      EncodedNumber t;
      t.set_double(phe_pub_key->n[0], plain_rand_weight_mat[i][j], precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, m_weight_mat[i][j], t);
    }
  }
  if (m_fit_bias) {
    for (int j = 0; j < m_num_outputs; j++) {
      EncodedNumber t;
      t.set_double(phe_pub_key->n[0], plain_rand_bias_vec[j], precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, m_bias[j], t);
    }
  }

  djcs_t_free_public_key(phe_pub_key);
}

void Layer::comp_1st_layer_agg_output(const Party &party,
                                      int cur_batch_size,
                                      const std::vector<int>& local_weight_sizes,
                                      EncodedNumber **encoded_batch_samples,
                                      int output_size,
                                      EncodedNumber **res) const {
  log_info("[comp_1st_layer_agg_output] compute the encrypted aggregation for the first hidden layer");
  if (m_num_outputs != output_size) {
    log_error("[comp_1st_layer_agg_output] neuron size does not match");
    exit(EXIT_FAILURE);
  }
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int precision = std::abs(encoded_batch_samples[0][0].getter_exponent())
      + std::abs(m_weight_mat[0][0].getter_exponent());

  // each party has a subset of features of a sample
  // encoded_batch_samples' dim = {(cur_batch_size, n_feature_0), ..., (cur_batch_size, n_feature_{m-1})}
  // where n_feature_0, ..., n_feature_{m-1} are the number of features held by the corresponding party
  // thus, an easier way is to split the m_weight_mat into m blocks, and each party computes the
  // matrix multiplication between the encoded_batch_samples and a block, then the active party
  // aggregates the result from each party to get the overall aggregated output,
  // whose dim = (cur_batch_size, m_num_outputs).

  // extract local weights according to local_weight_sizes
  log_info("[comp_1st_layer_agg_output] extract local weights");
  int local_n_features = local_weight_sizes[party.party_id];
  log_info("[comp_1st_layer_agg_output] local_n_features = " + std::to_string(local_n_features));
  log_info("[comp_1st_layer_agg_output] m_num_outputs = " + std::to_string(m_num_outputs));
  for (int i = 0; i < local_weight_sizes.size(); i++) {
    log_info("[comp_1st_layer_agg_output] local_weight_sizes["
      + std::to_string(i) + "] = " + std::to_string(local_weight_sizes[i]));
  }
  int start_idx = 0;
  for (int i = 0; i < party.party_id; i++) {
    start_idx += local_weight_sizes[i];
  }
  log_info("[comp_1st_layer_agg_output] start_idx = " + std::to_string(start_idx));
  // each party init a block of weight matrix whose dim = (local_weight_size, m_num_outputs)
  auto** local_weight_mat = new EncodedNumber*[local_n_features];
  for (int i = 0; i < local_n_features; i++) {
    local_weight_mat[i] = new EncodedNumber[m_num_outputs];
  }
  for (int i = 0; i < local_n_features; i++) {
    for (int j = 0; j < m_num_outputs; j++) {
      local_weight_mat[i][j] = m_weight_mat[start_idx+i][j];
    }
  }
  // each party compute matrix multiplication between encoded_batch_samples and local_weight_mat
  log_info("[comp_1st_layer_agg_output] compute local aggregation");
  auto** local_mat_mul_res = new EncodedNumber*[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    local_mat_mul_res[i] = new EncodedNumber[m_num_outputs];
  }
  djcs_t_aux_mat_mat_ep_mult(phe_pub_key, party.phe_random,
                             local_mat_mul_res,
                             local_weight_mat,
                             encoded_batch_samples,
                             local_n_features,
                             m_num_outputs,
                             cur_batch_size,
                             local_n_features);

  // the active party aggregate the result and broadcast
  if (party.party_type == falcon::ACTIVE_PARTY) {
    log_info("[comp_1st_layer_agg_output] the active party aggregate");
    // copy self local_mat_mul_res
    for (int i = 0; i < cur_batch_size; i++) {
      for (int j = 0; j < m_num_outputs; j++) {
        res[i][j] = local_mat_mul_res[i][j];
      }
    }
    // receive and aggregate
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        // reuse the local_mat_mul_res object to restore the received encoded matrix
        log_info("[comp_1st_layer_agg_output] receive encoded matrix from party " + std::to_string(id));
        std::string recv_id_mat_str;
        party.recv_long_message(id, recv_id_mat_str);
        deserialize_encoded_number_matrix(local_mat_mul_res, cur_batch_size,
                                          m_num_outputs, recv_id_mat_str);
        // homomorphic addition between local_mat_mul_res and res
        djcs_t_aux_matrix_ele_wise_ee_add(phe_pub_key, res,
                                          res, local_mat_mul_res,
                                          cur_batch_size, m_num_outputs);
      }
    }
    // if fit_bias, add the bias to each output neuron of each sample
    if (m_fit_bias) {
      int prec_res = std::abs(res[0][0].getter_exponent());
      int prec_bias = std::abs(m_bias[0].getter_exponent());
      log_info("[comp_1st_layer_agg_output] prec_res = " + std::to_string(prec_res));
      log_info("[comp_1st_layer_agg_output] prec_bias = " + std::to_string(prec_bias));
      auto* m_bias_inc = new EncodedNumber[m_num_outputs];
      djcs_t_aux_increase_prec_vec(phe_pub_key, m_bias_inc, prec_res, m_bias, m_num_outputs);
      for (int i = 0; i < cur_batch_size; i++) {
        for (int j = 0; j < m_num_outputs; j++) {
          djcs_t_aux_ee_add(phe_pub_key, res[i][j], res[i][j], m_bias_inc[j]);
        }
      }
      delete [] m_bias_inc;
    }
  } else {
    log_info("[comp_1st_layer_agg_output] send local_mat_mul_res to active party");
    log_info("[comp_1st_layer_agg_output] cur_batch_size = " + std::to_string(cur_batch_size));
    log_info("[comp_1st_layer_agg_output] m_num_outputs = " + std::to_string(m_num_outputs));
    std::string local_mat_str;
    serialize_encoded_number_matrix(local_mat_mul_res, cur_batch_size,
                                    m_num_outputs, local_mat_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_mat_str);
  }
  broadcast_encoded_number_matrix(
      party, res,
      cur_batch_size,
      m_num_outputs,
      ACTIVE_PARTY_ID);

  // free memory
  for (int i = 0; i < local_n_features; i++) {
    delete [] local_weight_mat[i];
  }
  delete [] local_weight_mat;
  for (int i = 0; i < cur_batch_size; i++) {
    delete [] local_mat_mul_res[i];
  }
  delete [] local_mat_mul_res;
  djcs_t_free_public_key(phe_pub_key);
}

void Layer::comp_other_layer_agg_output(const Party &party,
                                        int cur_batch_size,
                                        const std::vector<std::vector<double>> &prev_layer_outputs_shares,
                                        int output_size,
                                        EncodedNumber **res) const {
  // execution method:
  // 1. since the inputs are secret shares, let each party compute a partial aggregation,
  // 2. each party send the partial result to active party for fully aggregation,
  // 3. the active party add the bias term, and broadcast the encrypted outputs
  log_info("[comp_other_layer_agg_output] compute the encrypted aggregation for the other hidden layer");
  int prev_batch_size = (int) prev_layer_outputs_shares.size();
  int prev_output_size = (int) prev_layer_outputs_shares[0].size();
  if ((prev_output_size != m_num_inputs) ||
      (prev_batch_size != cur_batch_size) ||
      (output_size != m_num_outputs)) {
    log_error("[comp_other_layer_agg_output] size does not match");
    exit(EXIT_FAILURE);
  }
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // local aggregation
  // local shares * ciphers matrix multiplication result dim = (cur_batch_size, m_num_outputs)
  auto** local_ciphers_shares_mul_res = new EncodedNumber*[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    local_ciphers_shares_mul_res[i] = new EncodedNumber[m_num_outputs];
  }
  log_info("[comp_other_layer_agg_output] prev_batch_size = " + std::to_string(prev_batch_size));
  log_info("[comp_other_layer_agg_output] prev_output_size = " + std::to_string(prev_output_size));
  log_info("[comp_other_layer_agg_output] m_num_inputs = " + std::to_string(m_num_inputs));
  log_info("[comp_other_layer_agg_output] m_num_outputs = " + std::to_string(m_num_outputs));
  cipher_shares_mat_mul(party, prev_layer_outputs_shares,
                        m_weight_mat, prev_batch_size, prev_output_size,
                        m_num_inputs, m_num_outputs, local_ciphers_shares_mul_res);
  log_info("[comp_other_layer_agg_output] party local ciphers shares computation finished");

  // active party aggregate the results
  if (party.party_type == falcon::ACTIVE_PARTY) {
  // copy self local_mat_mul_res
    for (int i = 0; i < cur_batch_size; i++) {
      for (int j = 0; j < m_num_outputs; j++) {
        res[i][j] = local_ciphers_shares_mul_res[i][j];
      }
    }
    // receive and aggregate
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        // reuse the local_mat_mul_res object to restore the received encoded matrix
        std::string recv_id_mat_str;
        party.recv_long_message(id, recv_id_mat_str);
        deserialize_encoded_number_matrix(local_ciphers_shares_mul_res,
                                          cur_batch_size, m_num_outputs, recv_id_mat_str);
        // homomorphic addition between local_mat_mul_res and res
        djcs_t_aux_matrix_ele_wise_ee_add(phe_pub_key, res, res,
                                          local_ciphers_shares_mul_res,
                                          cur_batch_size, m_num_outputs);
      }
    }
    // if fit_bias, add the bias to each output neuron of each sample
    if (m_fit_bias) {
      int prec_res = std::abs(res[0][0].getter_exponent());
      int prec_bias = std::abs(m_bias[0].getter_exponent());
      auto* m_bias_inc = new EncodedNumber[m_num_outputs];
      djcs_t_aux_increase_prec_vec(phe_pub_key, m_bias_inc, prec_res, m_bias, m_num_outputs);
      for (int i = 0; i < cur_batch_size; i++) {
        for (int j = 0; j < m_num_outputs; j++) {
          djcs_t_aux_ee_add(phe_pub_key, res[i][j], res[i][j], m_bias_inc[j]);
        }
      }
      delete [] m_bias_inc;
    }
  } else {
    std::string local_mat_str;
    serialize_encoded_number_matrix(local_ciphers_shares_mul_res,
                                    cur_batch_size, m_num_outputs, local_mat_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_mat_str);
  }
  broadcast_encoded_number_matrix(party, res,
                                  cur_batch_size, m_num_outputs, ACTIVE_PARTY_ID);


  // free memory
  for (int i = 0; i < cur_batch_size; i++) {
    delete [] local_ciphers_shares_mul_res[i];
  }
  delete [] local_ciphers_shares_mul_res;
  djcs_t_free_public_key(phe_pub_key);
}

void Layer::update_encrypted_weights(const Party &party,
                                     const std::vector<double> &deriv_error,
                                     double m_learning_rate,
                                     std::vector<double> *deltas) {

}