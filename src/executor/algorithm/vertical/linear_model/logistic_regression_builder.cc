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
// Created by wuyuncheng on 14/11/20.
//

#include <Networking/ssl_sockets.h>
#include <ctime>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/common.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/utils/metric/classification.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <future>
#include <iomanip> // std::setprecision
#include <random>
#include <thread>
#include <utility>

LogisticRegressionBuilder::LogisticRegressionBuilder() = default;

LogisticRegressionBuilder::LogisticRegressionBuilder(
    LogisticRegressionParams lr_params, int m_weight_size,
    std::vector<std::vector<double>> m_training_data,
    std::vector<std::vector<double>> m_testing_data,
    std::vector<double> m_training_labels, std::vector<double> m_testing_labels,
    double m_training_accuracy, double m_testing_accuracy)
    : ModelBuilder(std::move(m_training_data), std::move(m_testing_data),
                   std::move(m_training_labels), std::move(m_testing_labels),
                   m_training_accuracy, m_testing_accuracy) {
  batch_size = lr_params.batch_size;
  max_iteration = lr_params.max_iteration;
  converge_threshold = lr_params.converge_threshold;
  with_regularization = lr_params.with_regularization;
  alpha = lr_params.alpha;
  // NOTE: with double-precision, learning_rate can be 0.005
  // previously with float-precision, learning_rate is default 0.1
  learning_rate = lr_params.alpha;
  decay = lr_params.decay;
  penalty = std::move(lr_params.penalty);
  optimizer = std::move(lr_params.optimizer);
  multi_class = std::move(lr_params.multi_class);
  metric = std::move(lr_params.metric);
  dp_budget = lr_params.dp_budget;
  // whether to fit the bias term
  fit_bias = lr_params.fit_bias;
  log_reg_model = LogisticRegressionModel(m_weight_size);
}

LogisticRegressionBuilder::~LogisticRegressionBuilder() = default;

void LogisticRegressionBuilder::init_encrypted_weights(const Party &party,
                                                       int precision) {
  init_encrypted_model_weights(party, log_reg_model.party_weight_sizes,
                               log_reg_model.local_weights, precision);
  log_info(
      "[init_encrypted_weights]: model weights precision "
      "is: " +
      std::to_string(abs(log_reg_model.local_weights[0].getter_exponent())));
}

void LogisticRegressionBuilder::backward_computation(
    const Party &party, const std::vector<std::vector<double>> &batch_samples,
    EncodedNumber *predicted_labels, const std::vector<int> &batch_indexes,
    int precision, EncodedNumber *encrypted_gradients) {

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // convert batch loss shares back to encrypted losses
  int cur_batch_size = (int)batch_indexes.size();
  auto *encrypted_batch_losses = new EncodedNumber[cur_batch_size];
  int predicted_label_precision =
      std::abs(predicted_labels[0].getter_exponent());
  auto *batch_true_labels = new EncodedNumber[cur_batch_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < cur_batch_size; i++) {
      batch_true_labels[i].set_double(phe_pub_key->n[0],
                                      training_labels[batch_indexes[i]],
                                      predicted_label_precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, batch_true_labels[i],
                         batch_true_labels[i]);
    }
  }

  compute_encrypted_residual(party, batch_indexes, batch_true_labels, precision,
                             predicted_labels, encrypted_batch_losses);

  log_info(
      "[backward_computation] the precision of encrypted_batch_losses is: " +
      std::to_string(std::abs(encrypted_batch_losses[0].getter_exponent())));

  // after calculate loss, compute [loss_i]*x_{ij}
  auto encoded_batch_samples = new EncodedNumber *[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
  }

  // update formula: [w_j]=[w_j]-lr*{(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}} +
  // 2 * \alpha * [w_j]} lr*(1/|B|) is the same for all sample values, thus can
  // be initialized if distributed train, ps only aggregate the encrypted
  // parameters if divide this->batch_size here; if not distributed train,
  // cur_batch_size = this->batch_size
  double lr_batch = learning_rate / this->batch_size;
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
                                             0 - lr_batch * batch_samples[i][j],
                                             PHE_FIXED_POINT_PRECISION);
    }
  }

  // calculate gradients
  // if not with_regularization, no need to convert truncated weights;
  // otherwise, need to convert truncated weights to ciphers for the update

  // compute common gradients
  auto common_gradients = new EncodedNumber[log_reg_model.weight_size];
  // update each local weight j
  for (int j = 0; j < log_reg_model.weight_size; j++) {
    EncodedNumber gradient;
    auto *batch_feature_j = new EncodedNumber[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      batch_feature_j[i] = encoded_batch_samples[i][j];
    }
    djcs_t_aux_inner_product(phe_pub_key, party.phe_random, gradient,
                             encrypted_batch_losses, batch_feature_j,
                             cur_batch_size);
    common_gradients[j] = gradient;

    delete[] batch_feature_j;
  }

  if (!with_regularization) {
    // if without regularization, directly assign the common gradients
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      encrypted_gradients[j] = common_gradients[j];
    }
  } else {
    // currently only support l2, the additional term is (- 2 * lr * \alpha *
    // [w_j]) notice that if using distributed train, then it equals to the ps
    // minus multiple times of the regularized gradients as each worker does
    // this computation once, better to tune the learning_rate and alpha to make
    // the training more reasonable
    auto *regularized_gradients = new EncodedNumber[log_reg_model.weight_size];

    if (penalty == "l2") {
      int common_gradients_precision =
          abs(common_gradients[0].getter_exponent());
      double constant = 0 - learning_rate * 2 * alpha;
      EncodedNumber encoded_constant;
      encoded_constant.set_double(phe_pub_key->n[0], constant,
                                  PHE_FIXED_POINT_PRECISION);
      // first compute the regularized gradients
      for (int j = 0; j < log_reg_model.weight_size; j++) {
        djcs_t_aux_ep_mul(phe_pub_key, regularized_gradients[j],
                          log_reg_model.local_weights[j], encoded_constant);
      }
      //  20220621: this is not necessary, can increase the exponents via
      //  djsc_t_aux_ee_add_ext
      //      // then truncate the regularized gradients to
      //      common_gradients_precision truncate_ciphers_precision(party,
      //      regularized_gradients,
      //                                 log_reg_model.weight_size,
      //                                 ACTIVE_PARTY_ID,
      //                                 common_gradients_precision);
      // lastly, add the regularized gradients to common gradients, and assign
      // back
      for (int j = 0; j < log_reg_model.weight_size; j++) {
        djcs_t_aux_ee_add_ext(phe_pub_key, encrypted_gradients[j],
                              common_gradients[j], regularized_gradients[j]);
      }
    }

    // if l1 regularization, the other item has two forms:
    //    when w_j > 0, it should be { - lr * \alpha }
    //    when w_j < 0, it should be { lr * \alpha }
    // we use spdz to check the sign of [local_weights]
    if (penalty == "l1") {
      compute_l1_regularized_grad(party, regularized_gradients);
      // then, add the second item to the common_gradients
      int common_gradients_precision =
          abs(common_gradients[0].getter_exponent());
      int regularized_gradients_precision =
          abs(regularized_gradients[0].getter_exponent());
      int plaintext_precision =
          common_gradients_precision - regularized_gradients_precision;
      double constant = learning_rate;
      EncodedNumber encoded_constant;
      encoded_constant.set_double(phe_pub_key->n[0], constant,
                                  PHE_FIXED_POINT_PRECISION);
      // then, add the second item to the common_gradients
      for (int j = 0; j < log_reg_model.weight_size; j++) {
        djcs_t_aux_ep_mul(phe_pub_key, regularized_gradients[j],
                          regularized_gradients[j], encoded_constant);
        djcs_t_aux_ee_add_ext(phe_pub_key, encrypted_gradients[j],
                              common_gradients[j], regularized_gradients[j]);
      }
    }
    delete[] regularized_gradients;

    //    else {
    //      log_error("The penalty " + penalty + " is not supported for logistic
    //      regression"); exit(EXIT_FAILURE);
    //    }
  }

  djcs_t_free_public_key(phe_pub_key);
  // hcs_free_random(phe_random);
  delete[] encrypted_batch_losses;
  delete[] batch_true_labels;
  for (int i = 0; i < cur_batch_size; i++) {
    delete[] encoded_batch_samples[i];
  }
  delete[] encoded_batch_samples;
  delete[] common_gradients;
}

void LogisticRegressionBuilder::compute_l1_regularized_grad(
    const Party &party, EncodedNumber *regularized_gradients) {
  // To compute the sign of each weight in the model, we do the following steps:
  // 1. active party aggregates the weight size vector of all parties and
  // broadcast
  // 2. active party organizes a global encrypted weight vector and broadcast
  // 3. all parties convert the global weight vector into secret shares
  // 4. all parties connect to spdz parties to compute the sign and receive
  // shares
  // 5. all parties convert the secret shares into encrypted sign vector
  // 6. each party picks the corresponding local gradients and multiply the
  // hyper-parameter

  log_info("[compute_l1_regularized_grad]: begin to compute l1 regularized "
           "gradients");

  // step 2
  int global_weight_size =
      std::accumulate(log_reg_model.party_weight_sizes.begin(),
                      log_reg_model.party_weight_sizes.end(), 0);
  auto *global_weights = new EncodedNumber[global_weight_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // first append its own local weights
    int global_idx = 0;
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      global_weights[global_idx] = log_reg_model.local_weights[j];
      global_idx++;
    }
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        int recv_weight_size = log_reg_model.party_weight_sizes[i];
        auto *recv_local_weights = new EncodedNumber[recv_weight_size];
        std::string recv_local_weights_str;
        party.recv_long_message(i, recv_local_weights_str);
        deserialize_encoded_number_array(recv_local_weights, recv_weight_size,
                                         recv_local_weights_str);
        for (int k = 0; k < recv_weight_size; k++) {
          global_weights[global_idx] = recv_local_weights[k];
          global_idx++;
        }
        delete[] recv_local_weights;
      }
    }
  } else {
    // serialize local weights and send to active party
    std::string local_weights_str;
    serialize_encoded_number_array(log_reg_model.local_weights,
                                   log_reg_model.weight_size,
                                   local_weights_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_weights_str);
  }
  // active party broadcast the global weights vector
  broadcast_encoded_number_array(party, global_weights, global_weight_size,
                                 ACTIVE_PARTY_ID);

  log_info("[compute_l1_regularized_grad]: finish aggregate and broadcast "
           "global weights");

  // step 3
  std::vector<double> global_weights_shares;
  int phe_precision = abs(global_weights[0].getter_exponent());
  ciphers_to_secret_shares(party, global_weights, global_weights_shares,
                           global_weight_size, ACTIVE_PARTY_ID, phe_precision);

  log_info("[compute_l1_regularized_grad]: finish convert to secret shares");

  // step 4
  // the spdz_logistic_function_computation will do the extracting sign with
  // *(-1) operation
  falcon::SpdzLogRegCompType comp_type = falcon::L1_REGULARIZATION;
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_thread(
      spdz_logistic_function_computation, party.party_num, party.party_id,
      party.executor_mpc_ports, SPDZ_PLAYER_PATH, party.host_names,
      global_weights_shares, global_weight_size, comp_type, &promise_values);

  std::vector<double> global_regularized_sign_shares = future_values.get();
  // main thread wait spdz_thread to finish
  spdz_thread.join();

  log_info("[compute_l1_regularized_grad]: finish connect to spdz parties and "
           "receive result shares");

  // step 5
  auto *global_regularized_grad = new EncodedNumber[global_weight_size];
  secret_shares_to_ciphers(party, global_regularized_grad,
                           global_regularized_sign_shares, global_weight_size,
                           ACTIVE_PARTY_ID, phe_precision);

  log_info("[compute_l1_regularized_grad]: finish convert regularized gradient "
           "shares to ciphers");

  // step 6
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  EncodedNumber regularization_hyper_param;
  regularization_hyper_param.set_double(phe_pub_key->n[0], alpha,
                                        PHE_FIXED_POINT_PRECISION);
  for (int j = 0; j < global_weight_size; j++) {
    djcs_t_aux_ep_mul(phe_pub_key, global_regularized_grad[j],
                      global_regularized_grad[j], regularization_hyper_param);
  }
  // assign to local regularized gradients
  int start_idx = 0;
  for (int i = 0; i < party.party_id; i++) {
    start_idx += log_reg_model.party_weight_sizes[i];
  }
  for (int j = 0; j < log_reg_model.weight_size; j++) {
    regularized_gradients[j] = global_regularized_grad[start_idx];
    start_idx++;
  }

  log_info("[compute_l1_regularized_grad]: finish assign to local regularized "
           "gradients");

  delete[] global_weights;
  delete[] global_regularized_grad;
  djcs_t_free_public_key(phe_pub_key);
}

void LogisticRegressionBuilder::update_encrypted_weights(
    Party &party, EncodedNumber *encrypted_gradients) {
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // update the j-th weight in local_weight vector
  // need to make sure that the exponents of inner_product
  // and local weights are the same
  log_info("[update_encrypted_weights] log_reg_model precision is: " +
           std::to_string(
               std::abs(log_reg_model.local_weights[0].getter_exponent())));
  for (int j = 0; j < log_reg_model.weight_size; j++) {
    djcs_t_aux_ee_add_ext(phe_pub_key, log_reg_model.local_weights[j],
                          log_reg_model.local_weights[j],
                          encrypted_gradients[j]);
  }
  // check if the precision exceed the max precision and truncate
  if (std::abs(log_reg_model.local_weights[0].getter_exponent()) >=
      PHE_MAXIMUM_PRECISION) {
    log_reg_model.truncate_weights_precision(party, PHE_FIXED_POINT_PRECISION);
  }
  djcs_t_free_public_key(phe_pub_key);
}

void LogisticRegressionBuilder::train(Party party) {
  /// The training stage consists of the following steps
  /// 1. each party init encrypted local weights, "local_weights" =
  /// [w1],[w2],...[wn]
  /// 2. iterative computation
  ///   2.1 randomly select a batch of indexes for current iteration
  ///       2.1.1. For active party:
  ///          a) randomly select a batch of indexes
  ///          b) serialize it and send to other parties
  ///        2.1.2. For passive party:
  ///           a) receive recv_batch_indexes_str from active parties
  ///   2.2 compute homomorphic aggregation on the batch samples:
  ///       eg, the party select m examples and have n features
  ///       "local_batch_phe_aggregation"(m*1) = { [w1]x11+[w2]x12+...+[wn]x1n,
  ///                                             [w1]x21+[w2]x22+...+[wn]x2n,
  ///                                                      ....
  ///                                             [w1]xm1+[w2]xm2+...+[wn]xmn,}
  ///       2.2.1. For active party:
  ///          a) receive batch_phe_aggregation string from other parties
  ///          b) add the received batch_phe_aggregation with current
  ///          batch_phe_aggregation
  ///                 after adding, the final batch_phe_aggregation =
  ///                     {   [w1]x11+[w2]x12+...+[wn]x1n + [w(n+1)]x1(n+1) +
  ///                     [wF]x1F,
  ///                         [w1]x21+[w2]x22+...+[wn]x2n + [w(n+1)]x2(n+1) +
  ///                         [wF]x2F,
  ///                                              ...
  ///                         [w1]xm1+[w2]xm2+...+[wn]xmn  + [w(n+1)]xm(n+1) +
  ///                         [wF]xmF, }
  ///                     = { [WX1], [WX2],...,[WXm]  }
  ///                 F is total number of features. m is number of examples
  ///           c) board-cast the final batch_phe_aggregation to other party
  ///        2.2.2. For passive party:
  ///           a) receive final batch_phe_aggregation string from active
  ///           parties
  ///   2.3 convert the batch "batch_phe_aggregation" to secret shares, Whole
  ///   process follow Algorithm 1 in paper "Privacy Preserving Vertical
  ///   Federated Learning for Tree-based Model"
  ///       2.3.1. For active party:
  ///          a) randomly chooses ri belongs and encrypts it as [ri]
  ///          b) collect other party's encrypted_shares_str, eg [ri]
  ///          c) computes [e] = [batch_phe_aggregation[i]] + [r1]+...+[rk] (k
  ///          parties) d) board-cast the complete aggregated_shares_str to
  ///          other party e) collaborative decrypt the aggregated shares,
  ///          clients jointly decrypt [e] f) set secret_shares[i] = e - r1 mod
  ///          q
  ///       2.3.2. For passive party:
  ///          a) send local encrypted_shares_str (serialized [ri]) to active
  ///          party b) receive the aggregated shares from active party c) the
  ///          same as active party eg,collaborative decrypt the aggregated
  ///          shares, clients jointly decrypt [e] d) set secret_shares[i] = -ri
  ///          mod q
  ///   2.4 connect to spdz parties, feed the batch_aggregation_shares and do
  ///   mpc computations to get the gradient [1/(1+e^(wx))],
  ///       which is also stored as shares.Name it as loss_shares
  ///   2.5 combine loss shares and update encrypted local weights carefully
  ///       2.5.1. For active party:
  ///          a) collect other party's loss shares
  ///          c) aggregate other party's encrypted loss shares with local loss
  ///          shares, and deserialize it to get [1/(1+e^(wx))] d) board-cast
  ///          the dest_ciphers to other party e) calculate loss_i: [yi] +
  ///          [-1/(1+e^(Wxi))] for each example i f) board-cast the
  ///          encrypted_batch_losses_str to other party g) update the local
  ///          weight using [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|}
  ///          [loss_i]*x_{ij}}
  ///       2.5.2. For passive party:
  ///          a) send local encrypted_shares_str to active party
  ///          b) receive the dest_ciphers from active party
  ///          c) update the local weight using
  ///          [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}}
  /// 3. decrypt weights ciphertext

  log_info("************* Training Start *************");
  log_info("[train]: mpc_port_array_size after: " +
           std::to_string(party.executor_mpc_ports.size()));

  struct timespec training_start_time;
  clock_gettime(CLOCK_MONOTONIC, &training_start_time);

  if (optimizer != "sgd") {
    log_error("The " + optimizer + " optimizer does not supported");
    exit(EXIT_FAILURE);
  }

  // step 1: init encrypted local weights
  // (here use precision for consistence in the following)
  log_reg_model.sync_up_weight_sizes(party);
  int encrypted_weights_precision = PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;
  init_encrypted_weights(party, encrypted_weights_precision);
  log_info("Init encrypted local weights success");

  // record training data ids in data_indexes for iteratively batch selection
  std::vector<int> train_data_indexes;
  for (int i = 0; i < training_data.size(); i++) {
    train_data_indexes.push_back(i);
  }
  std::vector<std::vector<int>> batch_iter_indexes =
      precompute_iter_batch_idx(batch_size, max_iteration, train_data_indexes);

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // step 2: iteratively computation
  for (int iter = 0; iter < max_iteration; iter++) {
    log_info("-------- Iteration " + std::to_string(iter) + " --------");
    struct timespec iter_start_time;
    clock_gettime(CLOCK_MONOTONIC, &iter_start_time);

    // select batch_index
    std::vector<int> batch_indexes =
        sync_batch_idx(party, batch_size, batch_iter_indexes[iter]);
    log_info("-------- Iteration " + std::to_string(iter) +
             ", select_batch_idx success --------");
    // get training data with selected batch_index
    std::vector<std::vector<double>> batch_samples;
    for (int index : batch_indexes) {
      batch_samples.push_back(training_data[index]);
    }
    log_info("batch_samples[0][0] = " + std::to_string(batch_samples[0][0]));

    // encode the training data
    int cur_sample_size = (int)batch_samples.size();
    auto **encoded_batch_samples = new EncodedNumber *[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
    }
    encode_samples(party, batch_samples, encoded_batch_samples);

    log_info("-------- Iteration " + std::to_string(iter) +
             ", encode training data success --------");

    // compute predicted label
    auto *predicted_labels = new EncodedNumber[batch_indexes.size()];
    // 4 * fixed precision
    // weight * xj => encrypted_weights_precision + plaintext_samples_precision
    // (need to automatically adjust)
    int encrypted_batch_agg_precision =
        encrypted_weights_precision + plaintext_samples_precision;
    log_reg_model.forward_computation(
        party, cur_sample_size, encoded_batch_samples,
        encrypted_batch_agg_precision, predicted_labels);

    log_info("-------- Iteration " + std::to_string(iter) +
             ", forward computation success --------");
    log_info("The precision of predicted_labels is: " +
             std::to_string(abs(predicted_labels[0].getter_exponent())));

    // step 2.5: update encrypted local weights
    std::vector<double> truncated_weights_shares;
    auto *encrypted_gradients = new EncodedNumber[log_reg_model.weight_size];
    //    // need to make sure that update_precision * 2 =
    //    encrypted_weights_precision int update_precision =
    //    encrypted_weights_precision / 2;
    log_info("encrypted_batch_agg_precision is: " +
             std::to_string(encrypted_batch_agg_precision));

    backward_computation(party, batch_samples, predicted_labels, batch_indexes,
                         encrypted_batch_agg_precision, encrypted_gradients);
    log_info(
        "encrypted_gradients precision is: " +
        std::to_string(std::abs(encrypted_gradients[0].getter_exponent())));

    log_info("-------- Iteration " + std::to_string(iter) +
             ", backward computation success --------");
    // note that after backward computation, the precision of encrypted
    // gradients is 4 * PHE_FIXED_POINT_PRECISION, need to truncate
    //    party.truncate_ciphers_precision(encrypted_gradients,
    //                                     log_reg_model.weight_size,
    //                                     ACTIVE_PARTY_ID,
    //                                     encrypted_weights_precision);

    // 20220621: not needed, as can be handled by the update_encrypted_weights
    // function
    //    for (int i = 0; i < party.party_num; i++) {
    //      if (i == party.party_id) {
    //        broadcast_encoded_number_array(party, encrypted_gradients,
    //                                       log_reg_model.weight_size,
    //                                       party.party_id);
    //        truncate_ciphers_precision(party, encrypted_gradients,
    //                                   log_reg_model.weight_size,
    //                                   party.party_id,
    //                                   encrypted_weights_precision);
    //      } else {
    //        int party_i_weight_size = log_reg_model.party_weight_sizes[i];
    //        auto* party_i_enc_grad = new EncodedNumber[party_i_weight_size];
    //        broadcast_encoded_number_array(party, party_i_enc_grad,
    //                                       party_i_weight_size, i);
    //        truncate_ciphers_precision(party, party_i_enc_grad,
    //                                   party_i_weight_size,
    //                                   i, encrypted_weights_precision);
    //        delete [] party_i_enc_grad;
    //      }
    //    }

    log_info("encrypted_weights_precision is: " +
             std::to_string(encrypted_weights_precision));
    log_info(
        "encrypted_gradients precision is: " +
        std::to_string(std::abs(encrypted_gradients[0].getter_exponent())));
    update_encrypted_weights(party, encrypted_gradients);
    log_info("-------- Iteration " + std::to_string(iter) +
             ", update_encrypted_weights computation success --------");

    struct timespec iter_finish_time;
    clock_gettime(CLOCK_MONOTONIC, &iter_finish_time);

    double iter_consumed_time =
        (double)(iter_finish_time.tv_sec - iter_start_time.tv_sec);
    iter_consumed_time +=
        (double)(iter_finish_time.tv_nsec - iter_start_time.tv_nsec) /
        1000000000.0;

    log_info("-------- The " + std::to_string(iter) +
             "-th "
             "iteration consumed time = " +
             std::to_string(iter_consumed_time));

#ifdef DEBUG
    // intermediate information display
    // (including the training loss and weights)
    // whether to print loss+weights and/or evaluation report
    // based on iter, DEBUG, and PRINT_EVERY
    bool display_info = false;
    if (iter == 0) {
      // display info on the 0-th iter
      display_info = true;
    } else if ((iter + 1) % PRINT_EVERY == 0) {
      // in debug mode print every PRINT_EVERY (default 500) iters
      display_info = true;
      // in DEBUG mode, set PRINT_EVERY to 1 to
      // print out evaluation report for every iteration
    }

    if (display_info) {
      double training_loss = 0.0;
      loss_computation(party, falcon::TRAIN, training_loss);
      log_info(
          "DEBUG INFO: The " + std::to_string(iter) +
          "-th iteration training loss = " + std::to_string(training_loss));
      log_reg_model.display_weights(party);
      // print evaluation report
      if (iter != (max_iteration - 1)) {
        // but do not duplicate print for last iter
        eval(party, falcon::TRAIN, std::string());
      }
    }
#endif

    delete[] predicted_labels;
    delete[] encrypted_gradients;
    for (int i = 0; i < cur_sample_size; i++) {
      delete[] encoded_batch_samples[i];
    }
    delete[] encoded_batch_samples;
  }

  struct timespec training_finish_time;
  clock_gettime(CLOCK_MONOTONIC, &training_finish_time);

  double training_consumed_time =
      (double)(training_finish_time.tv_sec - training_start_time.tv_sec);
  training_consumed_time +=
      (double)(training_finish_time.tv_nsec - training_start_time.tv_nsec) /
      1000000000.0;

  log_info("Training time = " + std::to_string(training_consumed_time));
  log_info("************* Training Finished *************");
}

void LogisticRegressionBuilder::distributed_train(const Party &party,
                                                  const Worker &worker) {
  log_info("************* Distributed Training Start *************");

  struct timespec training_start_time;
  clock_gettime(CLOCK_MONOTONIC, &training_start_time);

  // (here use precision for consistence in the following)
  log_reg_model.sync_up_weight_sizes(party);
  int encrypted_weights_precision = PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;

  if (optimizer != "sgd") {
    log_error("The " + optimizer + " optimizer does not supported");
    exit(EXIT_FAILURE);
  }

  // required by spdz connector and mpc computation
  bigint::init_thread();

  for (int iter = 0; iter < max_iteration; iter++) {
    struct timespec iter_start_time;
    clock_gettime(CLOCK_MONOTONIC, &iter_start_time);

    log_info("-------- Worker Iteration " + std::to_string(iter) + " --------");
    // step 1.1: receive weight from master, and assign to current
    // log_reg_model.local_weights
    std::string weight_str;
    worker.recv_long_message_from_ps(weight_str);
    log_info("-------- Worker Iteration " + std::to_string(iter) +
             ", "
             "worker.receive weight success --------");

    auto *deserialized_weight = new EncodedNumber[log_reg_model.weight_size];
    deserialize_encoded_number_array(deserialized_weight,
                                     log_reg_model.weight_size, weight_str);
    for (int i = 0; i < log_reg_model.weight_size; i++) {
      this->log_reg_model.local_weights[i] = deserialized_weight[i];
    }

    // step 1.2: receive sample id from master, and assign batch_size to current
    // log_reg_model.batch_size
    std::string mini_batch_indexes_str;
    worker.recv_long_message_from_ps(mini_batch_indexes_str);
    std::vector<int> mini_batch_indexes;
    deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);
    // here should not record the current batch size in this->batch_size
    // this->batch_size = mini_batch_indexes.size();

    log_info("-------- "
             "Worker Iteration " +
             std::to_string(iter) +
             ", worker.receive sample id success, batch size " +
             std::to_string(mini_batch_indexes.size()) + " --------");

    // step 2.1: activate party send batch_indexes to other party, while other
    // party receive them. in distributed training, passive party use the index
    // sent by active party instead of the one sent by ps so overwrite
    // mini_batch_indexes mini_batch_indexes = select_batch_idx(party,
    // mini_batch_indexes);

    log_info("-------- "
             "Worker Iteration " +
             std::to_string(iter) + ", worker.consensus on batch ids" +
             " --------");
    log_info("print mini_batch_indexes");
    print_vector(mini_batch_indexes);

    // get training data with selected batch_index
    std::vector<std::vector<double>> mini_batch_samples;
    for (int index : mini_batch_indexes) {
      mini_batch_samples.push_back(training_data[index]);
    }

    // encode the training data
    int cur_sample_size = (int)mini_batch_samples.size();
    auto **encoded_mini_batch_samples = new EncodedNumber *[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_mini_batch_samples[i] =
          new EncodedNumber[log_reg_model.weight_size];
    }
    encode_samples(party, mini_batch_samples, encoded_mini_batch_samples);

    log_info("-------- "
             "Worker Iteration " +
             std::to_string(iter) + ", worker.encode_samples success" +
             " --------");

    auto *predicted_labels = new EncodedNumber[cur_sample_size];
    // compute predicted label
    int encrypted_batch_agg_precision =
        encrypted_weights_precision + plaintext_samples_precision;

    log_reg_model.forward_computation(
        party, cur_sample_size, encoded_mini_batch_samples,
        encrypted_batch_agg_precision, predicted_labels);

    log_info("-------- "
             "Worker Iteration " +
             std::to_string(iter) + ", forward computation success" +
             " --------");

    auto encrypted_gradients = new EncodedNumber[log_reg_model.weight_size];
    //    // need to make sure that update_precision * 2 =
    //    encrypted_weights_precision int update_precision =
    //    encrypted_weights_precision / 2;
    backward_computation(party, mini_batch_samples, predicted_labels,
                         mini_batch_indexes, encrypted_batch_agg_precision,
                         encrypted_gradients);

    log_info("-------- "
             "Worker Iteration " +
             std::to_string(iter) + ", backward computation success" +
             " --------");

    // step 2.5: send encrypted gradients to ps, ps will update weight
    std::string encrypted_gradients_str;
    serialize_encoded_number_array(encrypted_gradients,
                                   log_reg_model.weight_size,
                                   encrypted_gradients_str);
    worker.send_long_message_to_ps(encrypted_gradients_str);

    log_info("-------- "
             "Worker Iteration " +
             std::to_string(iter) + ", send gradients to ps success" +
             " --------");

    struct timespec iter_finish_time;
    clock_gettime(CLOCK_MONOTONIC, &iter_finish_time);

    double iter_consumed_time =
        (double)(iter_finish_time.tv_sec - iter_start_time.tv_sec);
    iter_consumed_time +=
        (double)(iter_finish_time.tv_nsec - iter_start_time.tv_nsec) /
        1000000000.0;

    log_info("-------- The " + std::to_string(iter) +
             "-th "
             "iteration consumed time = " +
             std::to_string(iter_consumed_time));

#ifdef DEBUG
    // intermediate information display
    // (including the training loss and weights)
    // whether to print loss+weights and/or evaluation report
    // based on iter, DEBUG, and PRINT_EVERY
    bool display_info = false;
    if (iter == 0) {
      // display info on the 0-th iter
      // display_info = true;
    } else if ((iter + 1) % PRINT_EVERY == 0) {
      // in debug mode print every PRINT_EVERY (default 500) iters
      display_info = true;
      // in DEBUG mode, set PRINT_EVERY to 1 to
      // print out evaluation report for every iteration
    }

    if (display_info) {
      double training_loss = 0.0;
      loss_computation(party, falcon::TRAIN, training_loss);
      log_info(
          "DEBUG INFO: The " + std::to_string(iter) +
          "-th iteration training loss = " + std::to_string(training_loss));
      log_reg_model.display_weights(party);
      // print evaluation report
      if (iter != (max_iteration - 1)) {
        // but do not duplicate print for last iter
        eval(party, falcon::TRAIN, std::string());
      }
    }
#endif

    delete[] deserialized_weight;
    for (int i = 0; i < cur_sample_size; i++) {
      delete[] encoded_mini_batch_samples[i];
    }
    delete[] predicted_labels;
    delete[] encoded_mini_batch_samples;
    delete[] encrypted_gradients;
  }

  struct timespec training_finish_time;
  clock_gettime(CLOCK_MONOTONIC, &training_finish_time);

  double training_consumed_time =
      (double)(training_finish_time.tv_sec - training_start_time.tv_sec);
  training_consumed_time +=
      (double)(training_finish_time.tv_nsec - training_start_time.tv_nsec) /
      1000000000.0;

  log_info("Distributed Training time = " +
           std::to_string(training_consumed_time));
  log_info("************* Distributed Training Finished *************");
}

void LogisticRegressionBuilder::eval(Party party, falcon::DatasetType eval_type,
                                     const std::string &report_save_path) {
  std::string dataset_str =
      (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str +
           " Start *************");

  struct timespec testing_start_time;
  clock_gettime(CLOCK_MONOTONIC, &testing_start_time);

  /// the testing workflow is as follows:
  ///     step 1: init test data
  ///     step 2: the parties call the model.predict function to compute
  ///     predicted labels step 3: active party aggregates and call
  ///     collaborative decryption step 4: active party computes the logistic
  ///     function and compare the clf metrics

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector<std::vector<double>> cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;

  // step 2: every party do the prediction, since all examples are required to
  // computed, there is no need communications of data index between different
  // parties
  auto *predicted_labels = new EncodedNumber[dataset_size];
  log_reg_model.predict(party, cur_test_dataset, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  auto *decrypted_labels = new EncodedNumber[dataset_size];
  collaborative_decrypt(party, predicted_labels, decrypted_labels, dataset_size,
                        ACTIVE_PARTY_ID);

  // std::cout << "Print predicted class" << std::endl;

  // step 4: active party computes the logistic function
  // and compare the clf metrics
  if (party.party_type == falcon::ACTIVE_PARTY) {
    eval_matrix_computation_and_save(decrypted_labels, dataset_size, eval_type,
                                     report_save_path);
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete[] predicted_labels;
  delete[] decrypted_labels;

  struct timespec testing_finish_time;
  clock_gettime(CLOCK_MONOTONIC, &testing_finish_time);

  double testing_consumed_time =
      (double)(testing_finish_time.tv_sec - testing_start_time.tv_sec);
  testing_consumed_time +=
      (double)(testing_finish_time.tv_nsec - testing_start_time.tv_nsec) /
      1000000000.0;

  log_info("Evaluation time = " + std::to_string(testing_consumed_time));
  log_info("************* Evaluation on " + dataset_str +
           " Finished *************");
}

void LogisticRegressionBuilder::distributed_eval(
    const Party &party, const Worker &worker, falcon::DatasetType eval_type) {
  std::string dataset_str =
      (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str +
           " Start *************");

  // step 1: retrieve current test data
  std::vector<std::vector<double>> cur_test_dataset =
      (eval_type == falcon::TRAIN) ? this->training_data : this->testing_data;

  log_info("current dataset size = " + std::to_string(cur_test_dataset.size()));

  // 2. each worker receive mini_batch_indexes_str from ps
  std::vector<int> mini_batch_indexes;
  std::string mini_batch_indexes_str;
  worker.recv_long_message_from_ps(mini_batch_indexes_str);
  deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);

  log_info("current mini batch size = " +
           std::to_string(mini_batch_indexes.size()));

  std::vector<std::vector<double>> cur_used_samples;
  for (const auto index : mini_batch_indexes) {
    cur_used_samples.push_back(cur_test_dataset[index]);
  }

  // 3: compute prediction result
  int cur_used_samples_size = (int)cur_used_samples.size();
  auto *predicted_labels = new EncodedNumber[cur_used_samples_size];
  log_reg_model.predict(party, cur_used_samples, predicted_labels);

  log_info("predict on mini batch samples finished");

  // 4: active party aggregates and call collaborative decryption
  auto *decrypted_labels = new EncodedNumber[cur_used_samples_size];
  collaborative_decrypt(party, predicted_labels, decrypted_labels,
                        cur_used_samples_size, ACTIVE_PARTY_ID);

  log_info("decrypt the predicted labels finished");

  // 5: ACTIVE_PARTY send to ps
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string decrypted_predict_label_str;
    serialize_encoded_number_array(decrypted_labels, cur_used_samples_size,
                                   decrypted_predict_label_str);
    worker.send_long_message_to_ps(decrypted_predict_label_str);
  }

  // 6: free resources
  delete[] predicted_labels;
  delete[] decrypted_labels;
}

void LogisticRegressionBuilder::eval_matrix_computation_and_save(
    EncodedNumber *decrypted_labels, int sample_number,
    falcon::DatasetType eval_type, const std::string &report_save_path) {

  std::string dataset_str =
      (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  // Classification Metrics object for performance metrics
  ClassificationMetrics clf_metrics;

  if (metric == "acc") {
    // the output is a vector of integers (predicted classes)
    int pred_class;
    std::vector<int> pred_classes;

    for (int i = 0; i < sample_number; i++) {
      double est_prob; // estimated probability
      // prediction and label classes
      // for binary classifier, positive and negative classes
      int positive_class = 1;
      int negative_class = 0;
      // decoded t score is already the probability
      // logistic function is a sigmoid function (S-shaped)
      // logistic function outputs a double between 0 and 1
      decrypted_labels[i].decode(est_prob);
      // Logistic Regresison Model make its prediction
      pred_class = (est_prob >= LOGREG_THRES) ? positive_class : negative_class;
      pred_classes.push_back(pred_class);
      // LOG(INFO) << "sample " << i << "'s est_prob = " << est_prob << ", and
      // predicted class = " << pred_class;
    }
    if (eval_type == falcon::TRAIN) {
      clf_metrics.compute_metrics(pred_classes, training_labels);
    }
    if (eval_type == falcon::TEST) {
      clf_metrics.compute_metrics(pred_classes, testing_labels);
    }

    // write results to report
    std::ofstream outfile;
    if (!report_save_path.empty()) {
      outfile.open(report_save_path, std::ios_base::app);
      if (outfile) {
        outfile << "******** Evaluation Report on " << dataset_str
                << " ********\n";
      }
    }
    log_info("Classification Confusion Matrix on " + dataset_str + " is: ");
    clf_metrics.pretty_print_cm(outfile);
    log_info("Classification Report on " + dataset_str + " is: ");
    clf_metrics.classification_report(outfile);
    outfile.close();
  } else {
    log_error("The " + metric + " metric is not supported");
    exit(EXIT_FAILURE);
  }
}

void LogisticRegressionBuilder::loss_computation(
    Party party, falcon::DatasetType dataset_type, double &loss) {
  std::string dataset_str =
      (dataset_type == falcon::TRAIN ? "training dataset" : "testing dataset");

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: init test data
  int dataset_size = (dataset_type == falcon::TRAIN) ? (int)training_data.size()
                                                     : (int)testing_data.size();
  std::vector<std::vector<double>> cur_test_dataset =
      (dataset_type == falcon::TRAIN) ? training_data : testing_data;

  // step 2: every party computes partial phe summation
  // and sends to active party
  std::vector<std::vector<double>> batch_samples;
  for (int i = 0; i < dataset_size; i++) {
    batch_samples.push_back(cur_test_dataset[i]);
  }
  // homomorphic aggregation (the precision should be 4 * prec now)
  int plaintext_precision = PHE_FIXED_POINT_PRECISION;
  auto *encrypted_aggregation = new EncodedNumber[dataset_size];

  // encode examples
  EncodedNumber **encoded_batch_samples;
  int cur_sample_size = (int)batch_samples.size();
  encoded_batch_samples = new EncodedNumber *[cur_sample_size];
  for (int i = 0; i < cur_sample_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
  }
  encode_samples(party, batch_samples, encoded_batch_samples);

  log_reg_model.compute_batch_phe_aggregation(
      party, cur_sample_size, encoded_batch_samples, plaintext_precision,
      encrypted_aggregation);

  // step 3: active party aggregates and call collaborative decryption
  auto *decrypted_aggregation = new EncodedNumber[dataset_size];
  collaborative_decrypt(party, encrypted_aggregation, decrypted_aggregation,
                        dataset_size, ACTIVE_PARTY_ID);

  // step 4: active party computes the logistic function
  // and compare the accuracy
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // the output is a vector of integers (predicted classes)
    std::vector<double> pred_probs;
    for (int i = 0; i < dataset_size; i++) {
      double logit;    // logit or t
      double est_prob; // estimated probability
      // prediction and label classes
      decrypted_aggregation[i].decode(logit);
      // decoded t score is called the logit
      // now input logit t to the logistic function
      // logistic function is a sigmoid function (S-shaped)
      // logistic function outputs a double between 0 and 1
      est_prob = logistic_function(logit);
      // Logistic Regresison Model make its prediction
      pred_probs.push_back(est_prob);
    }
    if (dataset_type == falcon::TRAIN) {
      loss = logistic_regression_loss(pred_probs, training_labels);
    }
    if (dataset_type == falcon::TEST) {
      loss = logistic_regression_loss(pred_probs, testing_labels);
    }
    log_info("The loss on " + dataset_str + " is: " + std::to_string(loss));
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete[] encrypted_aggregation;
  delete[] decrypted_aggregation;

  for (int i = 0; i < cur_sample_size; i++) {
    delete[] encoded_batch_samples[i];
  }
  delete[] encoded_batch_samples;
}
