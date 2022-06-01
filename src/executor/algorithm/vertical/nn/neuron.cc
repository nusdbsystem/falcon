//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/vertical/nn/neuron.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <random>

Neuron::Neuron() {
  m_num_inputs = 0;
  m_fit_bias = true;
}

Neuron::Neuron(int num_inputs, bool with_bias) {
  m_num_inputs = num_inputs;
  m_fit_bias = with_bias;
  m_bias = new EncodedNumber[1];
  m_weights = new EncodedNumber[m_num_inputs];
}

Neuron::Neuron(const Neuron &neuron) {
  m_num_inputs = neuron.m_num_inputs;
  m_fit_bias = neuron.m_fit_bias;
  m_bias = new EncodedNumber[1];
  m_weights = new EncodedNumber[m_num_inputs];
  m_bias[0] = neuron.m_bias[0];
  for (int i = 0; i < m_num_inputs; i++) {
    m_weights[i] = neuron.m_weights[i];
  }
}

Neuron &Neuron::operator=(const Neuron &neuron) {
  m_num_inputs = neuron.m_num_inputs;
  m_fit_bias = neuron.m_fit_bias;
  m_bias = new EncodedNumber[1];
  m_weights = new EncodedNumber[m_num_inputs];
  m_bias[0] = neuron.m_bias[0];
  for (int i = 0; i < m_num_inputs; i++) {
    m_weights[i] = neuron.m_weights[i];
  }
}

Neuron::~Neuron() {
  m_num_inputs = 0;
  m_fit_bias = true;
  delete [] m_bias;
  delete [] m_weights;
}

void Neuron::init_encrypted_weights(const Party &party, int precision) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  double limit = sqrt(2.0 / (double) m_num_inputs);
  std::mt19937 mt(RANDOM_SEED);
  std::uniform_real_distribution<double> dist(-limit, limit);
  std::vector<double> plain_rand_weights;
  // generate m_num_inputs + 1 random numbers, where the last is reserved for bias
  for (int i = 0; i < m_num_inputs + 1; i++) {
    double r = dist(mt);
    plain_rand_weights.push_back(r);
  }
  // assign plain_rand_weights to m_weights
  for (int i = 0; i < m_num_inputs; i++) {
    EncodedNumber t;
    t.set_double(phe_pub_key->n[0], plain_rand_weights[i], precision);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, m_weights[i], t);
  }
  // assign the last element to m_bias if m_fit_bias = true
  if (m_fit_bias) {
    EncodedNumber t;
    t.set_double(phe_pub_key->n[0], plain_rand_weights[m_num_inputs], precision);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, m_bias[0], t);
  }

  djcs_t_free_public_key(phe_pub_key);
}

void Neuron::compute_batch_phe_aggregation(const Party &party,
                                           int cur_batch_size,
                                           const std::vector<int>& local_weight_sizes,
                                           EncodedNumber **encoded_batch_samples,
                                           int precision,
                                           EncodedNumber *encrypted_batch_aggregation) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // extract local weights according to local_weight_sizes
  int local_weight_size = local_weight_sizes[party.party_id];
  auto* local_weights = new EncodedNumber[local_weight_size];
  int start_idx = 0;
  for (int i = 0; i < party.party_id; i++) {
    start_idx += local_weight_sizes[i];
  }
  for (int i = 0; i < local_weight_size; i++) {
    local_weights[i] = m_weights[start_idx + i];
  }

  // each party compute local homomorphic aggregation
  auto* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
  djcs_t_aux_vec_mat_ep_mult(phe_pub_key, party.phe_random,
                             local_batch_phe_aggregation, local_weights,
                             encoded_batch_samples, cur_batch_size, local_weight_size);

  sum_batch_local_aggregations(party, local_batch_phe_aggregation,
                               std::abs(encoded_batch_samples[0][0].getter_exponent()),
                               cur_batch_size,
                               encrypted_batch_aggregation);

  djcs_t_free_public_key(phe_pub_key);
  delete [] local_batch_phe_aggregation;
  delete [] local_weights;
}

void Neuron::compute_batch_phe_aggregation_with_shares(const Party &party,
                                                       int cur_batch_size,
                                                       int prev_neuron_size,
                                                       EncodedNumber **encoded_batch_shares,
                                                       int precision,
                                                       EncodedNumber *encrypted_batch_aggregation) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // each party compute local homomorphic aggregation
  auto* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
  djcs_t_aux_vec_mat_ep_mult(phe_pub_key, party.phe_random,
                             local_batch_phe_aggregation, m_weights,
                             encoded_batch_shares, cur_batch_size, prev_neuron_size);

  sum_batch_local_aggregations(party, local_batch_phe_aggregation,
                               std::abs(encoded_batch_shares[0][0].getter_exponent()),
                               cur_batch_size,
                               encrypted_batch_aggregation);

  djcs_t_free_public_key(phe_pub_key);
  delete [] local_batch_phe_aggregation;
}

void Neuron::sum_batch_local_aggregations(const Party &party,
                                          EncodedNumber *batch_local_aggregation,
                                          int precision,
                                          int cur_batch_size,
                                          EncodedNumber* encrypted_batch_aggregation) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // every party sends the local aggregation to the active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self local aggregation results
    for (int i = 0; i < cur_batch_size; i++) {
      // each element (i) in local_batch_phe_aggregation is [W].Xi,
      // Xi is a part of feature vector in example i
      encrypted_batch_aggregation[i] = batch_local_aggregation[i];
    }

    // 1. active party receive serialized string from other parties
    // deserialize and sum to batch_phe_aggregation
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        auto* recv_batch_phe_aggregation =
            new EncodedNumber[cur_batch_size];
        std::string recv_local_aggregation_str;
        party.recv_long_message(id, recv_local_aggregation_str);
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
    // 2. active party add bias term if m_fit_bias = true
    if (m_fit_bias) {
      EncodedNumber t;
      t.set_double(phe_pub_key->n[0], 1.0, precision);
      djcs_t_aux_ep_mul(phe_pub_key, t, m_bias[0], t);
      for (int i = 0; i < cur_batch_size; i++) {
        djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_aggregation[i],
                          encrypted_batch_aggregation[i], t);
      }
    }

    // 3. active party serialize and send the aggregated
    // batch_phe_aggregation to other parties
    std::string global_aggregation_str;
    serialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                   global_aggregation_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, global_aggregation_str);
      }
    }
  } else {
    // if it's passive party, it will not do the aggregation,
    // 1. it just serializes local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(batch_local_aggregation, cur_batch_size,
                                   local_aggregation_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);
    // 2. passive party receive the global batch aggregation
    // from the active party
    std::string recv_global_aggregation_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
    deserialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                     recv_global_aggregation_str);
  }

  djcs_t_free_public_key(phe_pub_key);
}

void Neuron::update_encrypted_weights(const Party &party,
                                      const std::vector<double> &deriv_error,
                                      double m_learning_rate,
                                      std::vector<double> *deltas) {

}