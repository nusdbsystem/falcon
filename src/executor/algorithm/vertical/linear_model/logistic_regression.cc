//
// Created by wuyuncheng on 14/11/20.
//

#include <falcon/algorithm/vertical/linear_model/logistic_regression.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <ctime>
#include <random>

#include <glog/logging.h>

LogisticRegression::LogisticRegression() {}

LogisticRegression::LogisticRegression(int m_batch_size,
    int m_max_iteration,
    float m_converge_threshold,
    float m_alpha,
    float m_learning_rate,
    float m_decay,
    int m_weight_size,
    std::string m_penalty,
    std::string m_optimizer,
    std::string m_multi_class,
    std::string m_metric,
    std::vector<std::vector<float> > m_training_data,
    std::vector<std::vector<float> > m_testing_data,
    std::vector<float> m_training_labels,
    std::vector<float> m_testing_labels,
    float m_training_accuracy,
    float m_testing_accuracy) : Model(std::move(m_training_data),
        std::move(m_testing_data),
        std::move(m_training_labels),
        std::move(m_testing_labels),
        m_training_accuracy,
        m_testing_accuracy) {
  batch_size = m_batch_size;
  max_iteration = m_max_iteration;
  converge_threshold = m_converge_threshold;
  alpha = m_alpha;
  learning_rate = m_learning_rate;
  decay = m_decay;
  weight_size = m_weight_size;
  penalty = std::move(m_penalty);
  optimizer = std::move(m_optimizer);
  multi_class = std::move(m_multi_class);
  metric = std::move(m_metric);
  local_weights = new EncodedNumber[weight_size];
}

LogisticRegression::~LogisticRegression() {
  delete [] local_weights;
}

void LogisticRegression::init_encrypted_weights(const Party &party, int precision) {
  LOG(INFO) << "Init encrypted local weights.";
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  hcs_random* phe_random = hcs_init_random();

  party.getter_phe_pub_key(phe_pub_key);
  party.getter_phe_random(phe_random);

  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<float> dist(0.0, 1.0);
  // srand(static_cast<unsigned> (time(nullptr)));
  for (int i = 0; i < weight_size; i++) {
    // generate random float values within (0, 1],
    // init fixed point EncodedNumber,
    // and encrypt with public key
    float v = dist(mt);
    // float v = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    EncodedNumber t;
    t.set_float(phe_pub_key->n[0], v, precision);
    djcs_t_aux_encrypt(phe_pub_key, phe_random, local_weights[i], t);
  }

  djcs_t_free_public_key(phe_pub_key);
  hcs_free_random(phe_random);
}

std::vector<int> LogisticRegression::select_batch_idx(const Party &party,
    std::vector<int> data_indexes) {
  std::vector<int> batch_indexes;
  // if active party, randomly select batch indexes and send to other parties
  // if passive parties, receive batch indexes and return
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
    for (int i = 0; i < batch_size; i++) {
      batch_indexes.push_back(data_indexes[i]);
    }
    // send to other parties
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        std::string batch_indexes_str;
        serialize_int_array(batch_indexes, batch_indexes_str);
        party.send_long_message(i, batch_indexes_str);
      }
    }
  } else {
    std::string recv_batch_indexes_str;
    party.recv_long_message(0, recv_batch_indexes_str);
    deserialize_int_array(batch_indexes, recv_batch_indexes_str);
  }
  return batch_indexes;
}

void LogisticRegression::compute_batch_phe_aggregation(const Party &party,
    std::vector<int> batch_indexes,
    EncodedNumber *batch_phe_aggregation) {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  hcs_random* phe_random = hcs_init_random();
  party.getter_phe_pub_key(phe_pub_key);
  party.getter_phe_random(phe_random);

  // TODO: add batch phe aggregation

  djcs_t_free_public_key(phe_pub_key);
  hcs_free_random(phe_random);
}