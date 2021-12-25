//
// Created by root on 11/13/21.
//

#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/metric/classification.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/logger/logger.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision
#include <utility>

#include <glog/logging.h>

// generate a batch (array) of data indexes according to the shuffle
// array size is the batch-size
// called in each new iteration
std::vector<int> sync_batch_idx(const Party& party,
                                int batch_size,
                                std::vector<int> cur_batch_indexes) {
  std::vector<int> batch_indexes;
  // if active party, randomly select batch indexes and send to other parties
  // if passive parties, receive batch indexes and return
  if (party.party_type == falcon::ACTIVE_PARTY) {
    log_info("ACTIVE_PARTY sync batch indexes");
    // send to other parties
    batch_indexes = std::move(cur_batch_indexes);
    std::string batch_indexes_str;
    serialize_int_array(batch_indexes, batch_indexes_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, batch_indexes_str);
      }
    }
  } else {
    std::string recv_batch_indexes_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_batch_indexes_str);
    deserialize_int_array(batch_indexes, recv_batch_indexes_str);
  }
  return batch_indexes;
}

std::vector<std::vector<int>> precompute_iter_batch_idx(int batch_size,
                                                        int max_iter,
                                                        std::vector<int> data_indexes) {
  std::vector<std::vector<int>> batch_iter_indexes;
  batch_iter_indexes.reserve(max_iter);
  std::default_random_engine rng(RANDOM_SEED);
  // shuffle and assign mini batch index for each iter
  for (int iter = 0; iter < max_iter; iter++) {
    log_info("------ batch indexes for iter " + std::to_string(iter) + " ------");
    // NOTE: cannot use a fixed seed here!!
    // select_batch_idx actually return a batch of indexes
    // cannot use a fixed seed, otherwise the data indexes are fixed
    // std::random_device rd;
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
    std::vector<int> batch_indexes;
    batch_indexes.reserve(batch_size);
    for (int i = 0; i < batch_size; i++) {
      log_info("data_indexes selected = " + std::to_string(data_indexes[i]));
      batch_indexes.push_back(data_indexes[i]);
    }
    batch_iter_indexes.push_back(batch_indexes);
  }
  return batch_iter_indexes;
}

void init_encrypted_model_weights(const Party& party,
                                  std::vector<int> party_weight_sizes,
                                  EncodedNumber* encrypted_vector,
                                  int precision) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  int global_weight_size = std::accumulate(party_weight_sizes.begin(), party_weight_sizes.end(), 0);
  double limit = sqrt(2.0 / (double) global_weight_size);
//  std::random_device rd;
  std::mt19937 mt(RANDOM_SEED);
  // initialization of weights
  // random initialization with a uniform range
  log_info("limit = " + std::to_string(limit));
  google::FlushLogFiles(google::INFO);
  std::uniform_real_distribution<double> dist(-limit, limit);
  // NOTE: for better compare to baseline that ensure the same randomness
  std::vector<double> global_weights;
  global_weights.reserve(global_weight_size);
  for (int i = 0; i < global_weight_size; i++) {
    double r = dist(mt);
    global_weights.push_back(r);
    log_info("global_weights[" + std::to_string(i) + "] = " + std::to_string(r));
  }
  // srand(static_cast<unsigned> (time(nullptr)));
  int start_idx = 0;
  for (int i = 0; i < party.party_num; i++) {
    if (i < party.party_id) {
      start_idx += party_weight_sizes[i];
    } else {
      break;
    }
  }
  int local_weight_size = party_weight_sizes[party.party_id];
  for (int i = 0; i < local_weight_size; i++) {
    // generate random double values within (-limit, limit),
    // init fixed point EncodedNumber,
    // and encrypt with public key
    double v = global_weights[start_idx];
    log_info("local_weights[" + std::to_string(i) + "] = " + std::to_string(v));
    // double v = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    EncodedNumber t;
    t.set_double(phe_pub_key->n[0], v, precision);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_vector[i], t);
    start_idx += 1;
  }
  djcs_t_free_public_key(phe_pub_key);
}

void split_dataset(Party* party,
                   bool fit_bias,
                   std::vector<std::vector<double> >& training_data,
                   std::vector<std::vector<double> >& testing_data,
                   std::vector<double>& training_labels,
                   std::vector<double>& testing_labels,
                   double split_percentage) {
  party->split_train_test_data(split_percentage, training_data, testing_data,
                               training_labels, testing_labels);
  int weight_size = party->getter_feature_num();
  log_info("original weight_size = " + std::to_string(weight_size));
  // retrieve the fit_bias term
  // if this is active party, and fit_bias is true
  // fit_bias or fit_intercept, for whether to plus the
  // constant _bias_ or _intercept_ term
  if ((party->party_type == falcon::ACTIVE_PARTY) && fit_bias) {
    log_info("fit_bias = TRUE, will insert x1=1 to features");
    // insert x1=1 to front of the features
    double x1 = BIAS_VALUE;
    for (int i = 0; i < training_data.size(); i++) {
      training_data[i].insert(training_data[i].begin(), x1);
    }
    for (int i = 0; i < testing_data.size(); i++) {
      testing_data[i].insert(testing_data[i].begin(), x1);
    }
    // update the new feature_num for the active party
    // also update the weight_size value +1
    // before passing weight_size to LogisticRegression instance below
    party->setter_feature_num(++weight_size);
    log_info("updated weight_size = " + std::to_string(weight_size));
    log_info("party getter feature_num = " + std::to_string(party->getter_feature_num()));
  }
  // for debug
  log_info("print the first training data before train");
  for (int j = 0; j < weight_size; j++) {
    log_info("vec[" + std::to_string(j) + "] = " + std::to_string(training_data[0][j]));
  }
  if (party->party_type == falcon::ACTIVE_PARTY) {
    log_info("first training data label = " + std::to_string(training_labels[0]));
  }
}

void compute_encrypted_residual(const Party& party,
                                const std::vector<int>& batch_indexes,
                                EncodedNumber* batch_true_labels,
                                int precision,
                                EncodedNumber* predicted_labels,
                                EncodedNumber* encrypted_batch_losses) {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // convert batch loss shares back to encrypted losses
  int cur_batch_size = (int) batch_indexes.size();
  // compute [f_t - y_t], i.e., [batch_losses]=[batch_outputs]-[batch_labels]
  // and broadcast active party use the label to calculate loss,
  // and sent it to other parties
  if (party.party_type == falcon::ACTIVE_PARTY) {
    int neg_one = -1;
    EncodedNumber EncodedNegOne;
    EncodedNegOne.set_integer(phe_pub_key->n[0], neg_one);
    for (int i = 0; i < cur_batch_size; i++) {
      // compute [0-y_t]
      djcs_t_aux_ep_mul(phe_pub_key,
                        batch_true_labels[i],
                        batch_true_labels[i],
                        EncodedNegOne);
      // compute phe addition [f_t - y_t]
      djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_losses[i],
                        predicted_labels[i],
                        batch_true_labels[i]);
    }
  }
  party.broadcast_encoded_number_array(encrypted_batch_losses,
                                       cur_batch_size, ACTIVE_PARTY_ID);
  log_info("Finish compute encrypted loss and sync up with all parties");

  djcs_t_free_public_key(phe_pub_key);
}
