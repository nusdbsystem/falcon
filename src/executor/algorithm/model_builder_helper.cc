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
#include <falcon/party/info_exchange.h>
#include <falcon/operator/conversion/op_conv.h>

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
#ifdef DEBUG
      if (i == 0) {
        log_info("data_indexes selected = " + std::to_string(data_indexes[i]));
      }
#endif
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
    EncodedNumber enc_neg_one;
    enc_neg_one.set_integer(phe_pub_key->n[0], neg_one);
    for (int i = 0; i < cur_batch_size; i++) {
      // compute [0-y_t]
      djcs_t_aux_ep_mul(phe_pub_key,
                        batch_true_labels[i],
                        batch_true_labels[i],
                        enc_neg_one);
      // compute phe addition [f_t - y_t]
      djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_losses[i],
                        predicted_labels[i],
                        batch_true_labels[i]);
    }
  }
  broadcast_encoded_number_array(party, encrypted_batch_losses,
                                 cur_batch_size, ACTIVE_PARTY_ID);
  log_info("Finish compute encrypted loss and sync up with all parties");

  djcs_t_free_public_key(phe_pub_key);
}

void encode_samples(const Party &party,
                    const std::vector<std::vector<double>> &used_samples,
                    EncodedNumber **encoded_samples,
                    int precision) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // retrieve batch samples and encode (notice to use cur_batch_size
  // instead of default batch size to avoid unexpected batch)
  int cur_sample_size = (int) used_samples.size();
  int cur_dimension = (int) used_samples[0].size();

  for (int i = 0; i < cur_sample_size; i++) {
    for (int j = 0; j < cur_dimension; j++) {
      encoded_samples[i][j].set_double(phe_pub_key->n[0],
                                       used_samples[i][j], precision);
    }
  }

  djcs_t_free_public_key(phe_pub_key);
}

void get_encrypted_2d_true_labels(
    const Party& party,
    int output_layer_size,
    const std::vector<double>& plain_batch_labels,
    EncodedNumber** batch_true_labels,
    int precision) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // note that the output_layer_size implicitly specifies regression or classification
  int cur_sample_size = (int) plain_batch_labels.size();
  for (int i = 0; i < cur_sample_size; i++) {
    if (output_layer_size == 1) {
      // regression
      batch_true_labels[i][0].set_double(phe_pub_key->n[0], plain_batch_labels[i], precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, batch_true_labels[i][0], batch_true_labels[i][0]);
    } else {
      // classification, the label is actually the index that to set 1.0
      int true_label_idx = (int) plain_batch_labels[i];
      for (int j = 0; j < output_layer_size; j++) {
        if (j == true_label_idx) {
          batch_true_labels[i][j].set_double(phe_pub_key->n[0], 1.0, precision);
        } else {
          batch_true_labels[i][j].set_double(phe_pub_key->n[0], 0.0, precision);
        }
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           batch_true_labels[i][j], batch_true_labels[i][j]);
      }
    }
  }
  djcs_t_free_public_key(phe_pub_key);
}

void display_encrypted_matrix(
    const Party& party,
    int row_size,
    int col_size,
    EncodedNumber** mat) {
  log_info("------------------ [display_encrypted_matrix] ---------------");
  // display m_weight_mat
  for (int i = 0; i < row_size; i++) {
    // decrypt layer l's m_weight_mat[i]
    auto* dec_weight_mat_i = new EncodedNumber[col_size];
    collaborative_decrypt(party, mat[i],
                          dec_weight_mat_i, col_size, ACTIVE_PARTY_ID);
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int j = 0; j < col_size; j++) {
        double w;
        dec_weight_mat_i[j].decode(w);
        log_info("[display_encrypted_matrix] matrix["
                     + std::to_string(i) + "][" + std::to_string(j) + "] = " + std::to_string(w));
      }
    }
    delete [] dec_weight_mat_i;
  }
}

void display_encrypted_vector(
    const Party& party,
    int size,
    EncodedNumber* vec) {
  log_info("------------------ [display_encrypted_vector] ---------------");
  // display m_bias
  auto* dec_bias_vec = new EncodedNumber[size];
  collaborative_decrypt(party, vec, dec_bias_vec, size, ACTIVE_PARTY_ID);
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int j = 0; j < size; j++) {
      double b;
      dec_bias_vec[j].decode(b);
      log_info("[display_encrypted_vector] vector[" + std::to_string(j) + "] = " + std::to_string(b));
    }
  }
  delete [] dec_bias_vec;
}

void display_shares_vector(
    const Party& party,
    const std::vector<double>& vec) {
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
}

void display_shares_matrix(
    const Party& party,
    const std::vector<std::vector<double>>& mat) {
  log_info("------------------ [display_shares_matrix] ---------------");
  // display secret share matrix
  std::vector<std::vector<double>> agg_mat(mat.size(), std::vector<double> (mat[0].size(), 0.0));
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
}