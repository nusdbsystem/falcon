//
// Created by wuyuncheng on 14/11/20.
//

#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/metric/classification.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/log_alg_params.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision
#include <utility>

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>


LogisticRegressionBuilder::LogisticRegressionBuilder() = default;

LogisticRegressionBuilder::LogisticRegressionBuilder(LogisticRegressionParams lr_params,
    int m_weight_size,
    std::vector<std::vector<double> > m_training_data,
    std::vector<std::vector<double> > m_testing_data,
    std::vector<double> m_training_labels,
    std::vector<double> m_testing_labels,
    double m_training_accuracy,
    double m_testing_accuracy) : ModelBuilder(std::move(m_training_data),
        std::move(m_testing_data),
        std::move(m_training_labels),
        std::move(m_testing_labels),
        m_training_accuracy,
        m_testing_accuracy) {
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

void LogisticRegressionBuilder::init_encrypted_weights(const Party &party, int precision) {
  log_info("Init encrypted local weights");
  init_encrypted_random_numbers(party, log_reg_model.weight_size,
                                log_reg_model.local_weights, precision);
  log_info("[init_encrypted_weights]: predicted_labels precision "
           "is: " + std::to_string(abs(log_reg_model.local_weights[0].getter_exponent())));
}

void LogisticRegressionBuilder::backward_computation(
    const Party& party,
    const std::vector<std::vector<double> >& batch_samples,
    EncodedNumber* predicted_labels,
    const std::vector<int>& batch_indexes,
    int precision,
    EncodedNumber* encrypted_gradients) {

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // convert batch loss shares back to encrypted losses
  int cur_batch_size = (int) batch_indexes.size();
  auto* encrypted_batch_losses = new EncodedNumber[cur_batch_size];
  compute_encrypted_residual(party, batch_indexes, training_labels,
                             precision, predicted_labels, encrypted_batch_losses);

  // after calculate loss, compute [loss_i]*x_{ij}
  auto encoded_batch_samples = new EncodedNumber*[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
  }

  // update formula: [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}} +
  // reg? lr*(1/|B|) is the same for all sample values, thus can be initialized
  // if distributed train, ps only aggregate the encrypted parameters if divide
  // this->batch_size here; if not distributed train, cur_batch_size = this->batch_size
  double lr_batch = learning_rate / this->batch_size;
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      // std::cout << "The " << i << "-th sample's " << j <<
      // "-th feature value = " << 0 - lr_batch * batch_samples[i][j] <<
      // std::endl;
      encoded_batch_samples[i][j].set_double(
          phe_pub_key->n[0], 0 - lr_batch * batch_samples[i][j], precision);
    }
  }

  // calculate gradients
  // if not with_regularization, no need to convert truncated weights;
  // otherwise, need to convert truncated weights to ciphers for the update
  if (!with_regularization) {
    // update each local weight j
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      EncodedNumber gradient;
      auto* batch_feature_j = new EncodedNumber[cur_batch_size];
      for (int i = 0; i < cur_batch_size; i++) {
        batch_feature_j[i] = encoded_batch_samples[i][j];
      }
      djcs_t_aux_inner_product(phe_pub_key, party.phe_random, gradient,
                               encrypted_batch_losses, batch_feature_j,
                               cur_batch_size);
      encrypted_gradients[j] = gradient;

      delete [] batch_feature_j;
    }
  } else {
    // TODO: handle the update formula when using regularization,
    // currently only l2
  }

  djcs_t_free_public_key(phe_pub_key);
  // hcs_free_random(phe_random);
  delete[] encrypted_batch_losses;
  for (int i = 0; i < cur_batch_size; i++) {
    delete[] encoded_batch_samples[i];
  }
  delete[] encoded_batch_samples;
}

void LogisticRegressionBuilder::update_encrypted_weights(
        Party& party,
        EncodedNumber* encrypted_gradients) const  {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // update the j-th weight in local_weight vector
  // need to make sure that the exponents of inner_product
  // and local weights are the same
  for (int j = 0; j < log_reg_model.weight_size; j++) {
      djcs_t_aux_ee_add(phe_pub_key,log_reg_model.local_weights[j],
                        log_reg_model.local_weights[j], encrypted_gradients[j]);
  }
  djcs_t_free_public_key(phe_pub_key);
}

void LogisticRegressionBuilder::train(Party party) {
  /// The training stage consists of the following steps
  /// 1. each party init encrypted local weights, "local_weights" = [w1],[w2],...[wn]
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
  ///          b) add the received batch_phe_aggregation with current batch_phe_aggregation
  ///                 after adding, the final batch_phe_aggregation =
  ///                     {   [w1]x11+[w2]x12+...+[wn]x1n + [w(n+1)]x1(n+1) + [wF]x1F,
  ///                         [w1]x21+[w2]x22+...+[wn]x2n + [w(n+1)]x2(n+1) + [wF]x2F,
  ///                                              ...
  ///                         [w1]xm1+[w2]xm2+...+[wn]xmn  + [w(n+1)]xm(n+1) + [wF]xmF, }
  ///                     = { [WX1], [WX2],...,[WXm]  }
  ///                 F is total number of features. m is number of examples
  ///           c) board-cast the final batch_phe_aggregation to other party
  ///        2.2.2. For passive party:
  ///           a) receive final batch_phe_aggregation string from active parties
  ///   2.3 convert the batch "batch_phe_aggregation" to secret shares, Whole process follow Algorithm 1 in paper "Privacy Preserving Vertical Federated Learning for Tree-based Model"
  ///       2.3.1. For active party:
  ///          a) randomly chooses ri belongs and encrypts it as [ri]
  ///          b) collect other party's encrypted_shares_str, eg [ri]
  ///          c) computes [e] = [batch_phe_aggregation[i]] + [r1]+...+[rk] (k parties)
  ///          d) board-cast the complete aggregated_shares_str to other party
  ///          e) collaborative decrypt the aggregated shares, clients jointly decrypt [e]
  ///          f) set secret_shares[i] = e - r1 mod q
  ///       2.3.2. For passive party:
  ///          a) send local encrypted_shares_str (serialized [ri]) to active party
  ///          b) receive the aggregated shares from active party
  ///          c) the same as active party eg,collaborative decrypt the aggregated shares, clients jointly decrypt [e]
  ///          d) set secret_shares[i] = -ri mod q
  ///   2.4 connect to spdz parties, feed the batch_aggregation_shares and do mpc computations to get the gradient [1/(1+e^(wx))],
  ///       which is also stored as shares.Name it as loss_shares
  ///   2.5 combine loss shares and update encrypted local weights carefully
  ///       2.5.1. For active party:
  ///          a) collect other party's loss shares
  ///          c) aggregate other party's encrypted loss shares with local loss shares, and deserialize it to get [1/(1+e^(wx))]
  ///          d) board-cast the dest_ciphers to other party
  ///          e) calculate loss_i: [yi] + [-1/(1+e^(Wxi))] for each example i
  ///          f) board-cast the encrypted_batch_losses_str to other party
  ///          g) update the local weight using [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}}
  ///       2.5.2. For passive party:
  ///          a) send local encrypted_shares_str to active party
  ///          b) receive the dest_ciphers from active party
  ///          c) update the local weight using [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}}
  /// 3. decrypt weights ciphertext

  log_info("************* Training Start *************");
  log_info("[train]: mpc_port_array_size after: " + std::to_string(party.executor_mpc_ports.size()));

  const clock_t training_start_time = clock();

  if (optimizer != "sgd") {
    log_error("The " + optimizer + " optimizer does not supported");
    exit(1);
  }

  // step 1: init encrypted local weights
  // (here use 3 * precision for consistence in the following)
  int encrypted_weights_precision = 3 * PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;
  init_encrypted_weights(party, encrypted_weights_precision);

  // record training data ids in data_indexes for iteratively batch selection
  std::vector<int> train_data_indexes;
  for (int i = 0; i < training_data.size(); i++) {
    train_data_indexes.push_back(i);
  }
  log_info("Init encrypted local weights success");

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // step 2: iteratively computation
  for (int iter = 0; iter < max_iteration; iter++) {
    log_info("-------- Iteration " + std::to_string(iter) + " --------");
    const clock_t iter_start_time = clock();

    // select batch_index
    std::vector< int> batch_indexes = select_batch_idx(party, batch_size, train_data_indexes);
    log_info("-------- Iteration " + std::to_string(iter) + ", select_batch_idx success --------");
    // get training data with selected batch_index
    std::vector<std::vector<double> > batch_samples;
    for (int index : batch_indexes) {
      batch_samples.push_back(training_data[index]);
    }

    std::cout << "batch_samples " << std::setprecision(17) << batch_samples[0][0] << std::endl;
    LOG(INFO) << "batch_samples " << std::setprecision(17) << batch_samples[0][0] ;

    // encode the training data
    int cur_sample_size = (int) batch_samples.size();
    auto** encoded_batch_samples = new EncodedNumber*[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
    }
    log_reg_model.encode_samples(party, batch_samples, encoded_batch_samples);

    // only for debug
    double v;
    encoded_batch_samples[0][0].decode(v);
    std::cout << "encoded_batch_samples " << std::setprecision(17) << v << std::endl;
    LOG(INFO) << "encoded_batch_samples " << std::setprecision(17) << v;

    log_info("-------- Iteration " + std::to_string(iter) + ", encode training data success --------");

    // compute predicted label
    auto *predicted_labels = new EncodedNumber[batch_indexes.size()];
    // 4 * fixed precision
    // weight * xj => encrypted_weights_precision + plaintext_samples_precision
    int encrypted_batch_aggregation_precision = encrypted_weights_precision + plaintext_samples_precision;
    log_reg_model.forward_computation(
        party,
        cur_sample_size,
        encoded_batch_samples,
        encrypted_batch_aggregation_precision,
        predicted_labels);

    log_info("-------- Iteration " + std::to_string(iter) + ", forward computation success --------");
    log_info("The precision of predicted_labels is: " + std::to_string(abs(predicted_labels[0].getter_exponent())));

    // step 2.5: update encrypted local weights
    std::vector<double> truncated_weights_shares;
    auto encrypted_gradients = new EncodedNumber[log_reg_model.weight_size];
    // need to make sure that update_precision * 2 = encrypted_weights_precision
    int update_precision = encrypted_weights_precision / 2;
    log_info("Update_precision is: " + std::to_string(update_precision));

    backward_computation(
        party,
        batch_samples,
        predicted_labels,
        batch_indexes,
        update_precision,
        encrypted_gradients);

    log_info("-------- Iteration " + std::to_string(iter) + ", backward computation success --------");
    update_encrypted_weights(party, encrypted_gradients);
    log_info("-------- Iteration " + std::to_string(iter) + ", update_encrypted_weights computation success --------");

    const clock_t iter_finish_time = clock();
    double iter_consumed_time =
        double(iter_finish_time - iter_start_time) / CLOCKS_PER_SEC;
    log_info("-------- The " + std::to_string(iter) + "-th "
                           "iteration consumed time = " + std::to_string(iter_consumed_time));

#if DEBUG
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
      LOG(INFO) << "DEBUG INFO: The " << iter
                << "-th iteration training loss = " << std::setprecision(17)
                << training_loss;
      std::cout << "DEBUG INFO: The " << iter
                << "-th iteration training loss = " << std::setprecision(17)
                << training_loss << std::endl;
      display_weights(party);
      // print evaluation report
      if (iter != (max_iteration - 1)) {
        // but do not duplicate print for last iter
        eval(party, falcon::TRAIN, std::string());
      }
    }
#endif

    delete [] predicted_labels;
    delete [] encrypted_gradients;
    for (int i = 0; i < cur_sample_size; i++) {
      delete [] encoded_batch_samples[i];
    }
    delete [] encoded_batch_samples;
  }

  const clock_t training_finish_time = clock();
  double training_consumed_time =
      double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("Training time = " + std::to_string(training_consumed_time));
  log_info("************* Training Finished *************");
}

void LogisticRegressionBuilder::distributed_train(
    const Party& party,
    const Worker& worker) {
  log_info("************* Distributed Training Start *************");
  const clock_t training_start_time = clock();
  // (here use 3 * precision for consistence in the following)
  int encrypted_weights_precision = 3 * PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;

  if (optimizer != "sgd") {
    log_error("The " + optimizer + " optimizer does not supported");
    exit(1);
  }

  // required by spdz connector and mpc computation
  bigint::init_thread();

  for (int iter = 0; iter < max_iteration; iter++) {
    log_info("-------- Worker Iteration " + std::to_string(iter) + " --------");
    // step 1.1: receive weight from master, and assign to current log_reg_model.local_weights
    std::string weight_str;
    worker.recv_long_message_from_ps(weight_str);
    log_info("-------- Worker Iteration " + std::to_string(iter) + ", "
                 "worker.receive weight success --------");

    auto* deserialized_weight = new EncodedNumber[log_reg_model.weight_size];
    deserialize_encoded_number_array(deserialized_weight, log_reg_model.weight_size, weight_str);
    for (int i = 0; i < log_reg_model.weight_size; i++) {
      this->log_reg_model.local_weights[i] = deserialized_weight[i];
    }

    // step 1.2: receive sample id from master, and assign batch_size to current log_reg_model.batch_size
    std::string mini_batch_indexes_str;
    worker.recv_long_message_from_ps(mini_batch_indexes_str);
    std::vector<int> mini_batch_indexes;
    deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);
    // here should not record the current batch size in this->batch_size
    // this->batch_size = mini_batch_indexes.size();

    log_info("-------- "
             "Worker Iteration "
             + std::to_string(iter)
             + ", worker.receive sample id success, batch size "
             + std::to_string(mini_batch_indexes.size()) + " --------");

    const clock_t iter_start_time = clock();

    // step 2.1: activate party send batch_indexes to other party, while other party receive them.
    // in distributed training, passive party use the index sent by active party instead of the one sent by ps
    // so overwrite mini_batch_indexes
    // mini_batch_indexes = select_batch_idx(party, mini_batch_indexes);

    log_info("-------- "
             "Worker Iteration "
             + std::to_string(iter)
             + ", worker.consensus on batch ids"
             + " --------");
    log_info("print mini_batch_indexes");
    print_vector(mini_batch_indexes);

    // get training data with selected batch_index
    std::vector<std::vector<double> > mini_batch_samples;
    for (int index : mini_batch_indexes) {
      mini_batch_samples.push_back(training_data[index]);
    }

    // encode the training data
    int cur_sample_size = (int) mini_batch_samples.size();
    auto** encoded_mini_batch_samples = new EncodedNumber*[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_mini_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
    }
    log_reg_model.encode_samples(party, mini_batch_samples, encoded_mini_batch_samples);

    log_info("-------- "
             "Worker Iteration "
             + std::to_string(iter)
             + ", worker.encode_samples success"
             + " --------");

    auto *predicted_labels = new EncodedNumber[cur_sample_size];
    // compute predicted label
    int encrypted_batch_aggregation_precision = encrypted_weights_precision + plaintext_samples_precision;

    log_reg_model.forward_computation(
        party,
        cur_sample_size,
        encoded_mini_batch_samples,
        encrypted_batch_aggregation_precision,
        predicted_labels);

    log_info("-------- "
             "Worker Iteration "
             + std::to_string(iter)
             + ", forward computation success"
             + " --------");

    auto encrypted_gradients = new EncodedNumber[log_reg_model.weight_size];
    // need to make sure that update_precision * 2 = encrypted_weights_precision
    int update_precision = encrypted_weights_precision / 2;
    backward_computation(party,
        mini_batch_samples,
        predicted_labels,
        mini_batch_indexes,
        update_precision,
        encrypted_gradients);

    log_info("-------- "
             "Worker Iteration "
             + std::to_string(iter)
             + ", backward computation success"
             + " --------");

    // step 2.5: send encrypted gradients to ps, ps will update weight
    std::string encrypted_gradients_str;
    serialize_encoded_number_array(encrypted_gradients, log_reg_model.weight_size, encrypted_gradients_str);
    worker.send_long_message_to_ps(encrypted_gradients_str);

    log_info("-------- "
             "Worker Iteration "
             + std::to_string(iter)
             + ", send gradients to ps success"
             + " --------");

    const clock_t iter_finish_time = clock();
    double iter_consumed_time =
            double(iter_finish_time - iter_start_time) / CLOCKS_PER_SEC;

    log_info("-------- The " + std::to_string(iter) + "-th "
                                                      "iteration consumed time = " + std::to_string(iter_consumed_time));

#if DEBUG
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
      LOG(INFO) << "DEBUG INFO: The " << iter
                << "-th iteration training loss = " << std::setprecision(17)
                << training_loss;
      std::cout << "DEBUG INFO: The " << iter
                << "-th iteration training loss = " << std::setprecision(17)
                << training_loss << std::endl;
      display_weights(party);
      // print evaluation report
      if (iter != (max_iteration - 1)) {
          // but do not duplicate print for last iter
          eval(party, falcon::TRAIN, std::string());
      }
    }
#endif

    delete [] deserialized_weight;
    for (int i = 0; i < cur_sample_size; i++) {
      delete [] encoded_mini_batch_samples[i];
    }
    delete [] predicted_labels;
    delete [] encoded_mini_batch_samples;
    delete [] encrypted_gradients;
  }

  const clock_t training_finish_time = clock();
  double training_consumed_time =
          double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("Distributed Training time = " + std::to_string(training_consumed_time));
  log_info("************* Distributed Training Finished *************");
}

void LogisticRegressionBuilder::eval(Party party,
    falcon::DatasetType eval_type,
    const std::string& report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str + " Start *************");
  const clock_t testing_start_time = clock();

  /// the testing workflow is as follows:
  ///     step 1: init test data
  ///     step 2: the parties call the model.predict function to compute predicted labels
  ///     step 3: active party aggregates and call collaborative decryption
  ///     step 4: active party computes the logistic function and compare the clf metrics

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector<std::vector<double>> cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;

  // step 2: every party do the prediction, since all examples are required to
  // computed, there is no need communications of data index between different parties
  auto* predicted_labels = new EncodedNumber[dataset_size];
  log_reg_model.predict(party, cur_test_dataset, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(predicted_labels,
      decrypted_labels,
      dataset_size,
      ACTIVE_PARTY_ID);

  // std::cout << "Print predicted class" << std::endl;

  // step 4: active party computes the logistic function
  // and compare the clf metrics
  if (party.party_type == falcon::ACTIVE_PARTY) {
    eval_matrix_computation_and_save(decrypted_labels, dataset_size, eval_type, report_save_path);
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete [] predicted_labels;
  delete [] decrypted_labels;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time =
      double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  log_info("Evaluation time = " + std::to_string(testing_consumed_time));
  log_info("************* Evaluation on " + dataset_str + " Finished *************");
}

void LogisticRegressionBuilder::distributed_eval(
    const Party &party,
    const Worker &worker,
    falcon::DatasetType eval_type){
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str + " Start *************");

  // step 1: retrieve current test data
  std::vector<std::vector<double> > cur_test_dataset =
      (eval_type == falcon::TRAIN) ? this->training_data : this->testing_data;

  log_info("current dataset size = " + std::to_string(cur_test_dataset.size()));

  // 2. each worker receive mini_batch_indexes_str from ps
  std::vector<int> mini_batch_indexes;
  std::string mini_batch_indexes_str;
  worker.recv_long_message_from_ps(mini_batch_indexes_str);
  deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);

  log_info("current mini batch size = " + std::to_string(mini_batch_indexes.size()));

  std::vector<std::vector<double>> cur_used_samples;
  for (const auto index: mini_batch_indexes){
    cur_used_samples.push_back(cur_test_dataset[index]);
  }

  // 3: compute prediction result
  int cur_used_samples_size = (int) cur_used_samples.size();
  auto* predicted_labels = new EncodedNumber[cur_used_samples_size];
  log_reg_model.predict(party, cur_used_samples, predicted_labels);

  log_info("predict on mini batch samples finished");

  // 4: active party aggregates and call collaborative decryption
  auto* decrypted_labels = new EncodedNumber[cur_used_samples_size];
  party.collaborative_decrypt(predicted_labels,
                              decrypted_labels,
                              cur_used_samples_size,
                              ACTIVE_PARTY_ID);

  log_info("decrypt the predicted labels finished");

  // 5: ACTIVE_PARTY send to ps
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string decrypted_predict_label_str;
    serialize_encoded_number_array(decrypted_labels, cur_used_samples_size, decrypted_predict_label_str);
    worker.send_long_message_to_ps(decrypted_predict_label_str);
  }

  // 6: free resources
  delete [] predicted_labels;
  delete [] decrypted_labels;
}

void LogisticRegressionBuilder::eval_matrix_computation_and_save(
    EncodedNumber* decrypted_labels,
    int sample_number,
    falcon::DatasetType eval_type,
    const std::string& report_save_path){

  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  // Classification Metrics object for performance metrics
  ClassificationMetrics ClfMetrics;

  if (metric == "acc") {
    // the output is a vector of integers (predicted classes)
    int pred_class;
    std::vector<int> pred_classes;

    for (int i = 0; i < sample_number; i++) {
      double est_prob;  // estimated probability
      // prediction and label classes
      // for binary classifier, positive and negative classes
      int positive_class = 1;
      int negative_class = 0;
      // decoded t score is already the probability
      // logistic function is a sigmoid function (S-shaped)
      // logistic function outputs a double between 0 and 1
      decrypted_labels[i].decode(est_prob);
      // Logistic Regresison Model make its prediction
      pred_class =
          (est_prob >= LOGREG_THRES) ? positive_class : negative_class;
      pred_classes.push_back(pred_class);
      // LOG(INFO) << "sample " << i << "'s est_prob = " << est_prob << ", and predicted class = " << pred_class;
    }
    if (eval_type == falcon::TRAIN) {
      ClfMetrics.compute_metrics(pred_classes, training_labels);
    }
    if (eval_type == falcon::TEST) {
      ClfMetrics.compute_metrics(pred_classes, testing_labels);
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
    ClfMetrics.pretty_print_cm(outfile);
    log_info("Classification Report on " + dataset_str + " is: ");
    ClfMetrics.classification_report(outfile);
    outfile.close();
  } else {
    log_error("The " + metric + " metric is not supported");
    exit(1);
  }
}


void LogisticRegressionBuilder::loss_computation(Party party, falcon::DatasetType dataset_type, double &loss) {
  std::string dataset_str = (dataset_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  const clock_t testing_start_time = clock();

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: init test data
  int dataset_size = (dataset_type == falcon::TRAIN) ? training_data.size()
                                                     : testing_data.size();
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
  auto* encrypted_aggregation = new EncodedNumber[dataset_size];

  // encode examples
  EncodedNumber** encoded_batch_samples;
  int cur_sample_size = batch_samples.size();
  encoded_batch_samples = new EncodedNumber*[cur_sample_size];
  for (int i = 0; i < cur_sample_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
  }
  log_reg_model.encode_samples( party, batch_samples, encoded_batch_samples);

  log_reg_model.compute_batch_phe_aggregation(party,
      batch_samples.size(),
      encoded_batch_samples,
      plaintext_precision, encrypted_aggregation);

  // step 3: active party aggregates and call collaborative decryption
  auto* decrypted_aggregation = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(encrypted_aggregation, decrypted_aggregation,
                              dataset_size, ACTIVE_PARTY_ID);

  // step 4: active party computes the logistic function
  // and compare the accuracy
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // the output is a vector of integers (predicted classes)
    std::vector<double> pred_probs;
    for (int i = 0; i < dataset_size; i++) {
      double logit;     // logit or t
      double est_prob;  // estimated probability
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
    LOG(INFO) << "The loss on " << dataset_str
              << " is: " << std::setprecision(17) << loss;
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete[] encrypted_aggregation;
  delete[] decrypted_aggregation;

  for (int i = 0; i < cur_sample_size; i++) {
    delete [] encoded_batch_samples[i];
  }
  delete [] encoded_batch_samples;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time =
      double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  log_info("Time for loss computation on " + dataset_str + " = " + std::to_string(testing_consumed_time));
}

// initializes the LogisticRegressionBuilder instance
// and run .train() .eval() methods, then save model
void train_logistic_regression(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train, Worker* worker) {
  log_info("Run train_logistic_regression");
  log_info("is_distributed_train = " + std::to_string(is_distributed_train));

  LogisticRegressionParams params;
  deserialize_lr_params(params, params_str);
  log_logistic_regression_params(params);

  // split train test data for party and populate the vectors
  std::vector<std::vector<double>> training_data;
  std::vector<std::vector<double>> testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;

  // if not distributed train, then the party split the data
  // otherwise, the party/worker receive the data and phe keys from ps
  if (is_distributed_train == 0) {
    double split_percentage = SPLIT_TRAIN_TEST_RATIO;
    split_dataset(party, params.fit_bias, training_data, testing_data,
                  training_labels, testing_labels, split_percentage);
  } else {
    // here should receive the train/test data/labels from ps
    std::string recv_training_data_str, recv_testing_data_str;
    std::string recv_training_labels_str, recv_testing_labels_str;
    worker->recv_long_message_from_ps(recv_training_data_str);
    worker->recv_long_message_from_ps(recv_testing_data_str);
    deserialize_double_matrix(training_data, recv_training_data_str);
    deserialize_double_matrix(testing_data, recv_testing_data_str);
    if (party->party_type == falcon::ACTIVE_PARTY) {
      worker->recv_long_message_from_ps(recv_training_labels_str);
      worker->recv_long_message_from_ps(recv_testing_labels_str);
      deserialize_double_array(training_labels, recv_training_labels_str);
      deserialize_double_array(testing_labels, recv_testing_labels_str);
    }
    // also, receive the phe keys from ps
    // and set these to the party
    std::string recv_phe_keys_str;
    log_info("begin to receive phe keys from ps ");

    worker->recv_long_message_from_ps(recv_phe_keys_str);

    log_info("received phe keys from ps: " + recv_phe_keys_str);

    party->load_phe_key_string(recv_phe_keys_str);
    // set the weight size and party feature num
    party->setter_feature_num((int) training_data[0].size());
  }

  log_info("training_data.size() = " + std::to_string(training_data.size()));
  log_info("training_data[0].size() = " + std::to_string(training_data[0].size()));
  log_info("testing_data.size() = " + std::to_string(testing_data.size()));
  log_info("testing_data[0].size() = " + std::to_string(testing_data[0].size()));
  log_info("training_labels.size() = " + std::to_string(training_labels.size()));
  log_info("testing_labels.size() = " + std::to_string(testing_labels.size()));
  log_info("Init logistic regression model");

  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;
  LogisticRegressionBuilder log_reg_model_builder(params,
      weight_size,
      training_data,
      testing_data,
      training_labels,
      testing_labels,
      training_accuracy,
      testing_accuracy);

  if (is_distributed_train == 0){
    log_reg_model_builder.train(*party);
    log_reg_model_builder.eval(*party, falcon::TRAIN, model_report_file);
    log_reg_model_builder.eval(*party, falcon::TEST, model_report_file);
    // save model and report
    auto* model_weights = new EncodedNumber[weight_size];
    std::string pb_lr_model_string;
    serialize_lr_model(log_reg_model_builder.log_reg_model, pb_lr_model_string);
    save_pb_model_string(pb_lr_model_string, model_save_file);
    // save_lr_model(log_reg_model_builder.log_reg_model, model_save_file);
    // save_training_report(log_reg_model.getter_training_accuracy(),
    //    log_reg_model.getter_testing_accuracy(),
    //    model_report_file);
    delete[] model_weights;
  } else {
    log_reg_model_builder.distributed_train(*party, *worker);
    // in is_distributed_train, parameter server will save the model.
    log_reg_model_builder.distributed_eval(*party, *worker, falcon::TRAIN);
    log_reg_model_builder.distributed_eval(*party, *worker, falcon::TEST);
  }
}

// for DEBUG
void LogisticRegressionBuilder::display_weights(Party party) {
  log_info("display local weights");
  if (party.party_type == falcon::ACTIVE_PARTY) {
    auto* decrypted_local_weights = new EncodedNumber[log_reg_model.weight_size];
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        std::string weight_str;
        serialize_encoded_number_array(log_reg_model.local_weights,
            log_reg_model.weight_size, weight_str);
        std::string size_str = std::to_string(log_reg_model.weight_size);
        party.send_long_message(i, size_str);
        party.send_long_message(i, weight_str);
      }
    }
    party.collaborative_decrypt(log_reg_model.local_weights,
        decrypted_local_weights,
        log_reg_model.weight_size,
        ACTIVE_PARTY_ID);
    for (int i = 0; i < log_reg_model.weight_size; i++) {
      double weight;
      decrypted_local_weights[i].decode(weight);
      std::cout << "local weight[" << i << "] = " << std::setprecision(17)
                << weight << std::endl;
      LOG(INFO) << "local weight[" << i << "] = " << std::setprecision(17)
                << weight;
    }
    delete[] decrypted_local_weights;
  } else {
    std::string recv_weight_str, recv_size_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_size_str);
    party.recv_long_message(ACTIVE_PARTY_ID, recv_weight_str);
    int weight_num = std::stoi(recv_size_str);
    auto* received_party_weights = new EncodedNumber[weight_num];
    auto* decrypted_party_weights = new EncodedNumber[weight_num];
    deserialize_encoded_number_array(received_party_weights, weight_num,
                                     recv_weight_str);
    party.collaborative_decrypt(received_party_weights, decrypted_party_weights,
                                weight_num, ACTIVE_PARTY_ID);
    delete[] received_party_weights;
    delete[] decrypted_party_weights;
  }
}

// TODO: this is not used currently
// print one ciphertext for debug
void LogisticRegressionBuilder::display_one_ciphertext(Party party, EncodedNumber *number) {
  if (party.party_type == falcon::ACTIVE_PARTY) {
    auto* decrypted_number = new EncodedNumber[1];
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        std::string ciphertext_str;
        serialize_encoded_number_array(number, 1, ciphertext_str);
        party.send_long_message(i, ciphertext_str);
      }
    }
    party.collaborative_decrypt(number, decrypted_number, 1, ACTIVE_PARTY_ID);
    double v;
    decrypted_number[0].decode(v);
    std::cout << "plaintext " << std::setprecision(17) << v << std::endl;
    LOG(INFO) << "plaintext " << std::setprecision(17) << v;
    delete[] decrypted_number;
  } else {
    std::string recv_ciphertext_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_ciphertext_str);
    auto* recv_ciphertext = new EncodedNumber[1];
    auto* decrypted_ciphertext = new EncodedNumber[1];
    deserialize_encoded_number_array(recv_ciphertext, 1, recv_ciphertext_str);
    party.collaborative_decrypt(recv_ciphertext, decrypted_ciphertext, 1,
                                ACTIVE_PARTY_ID);
    delete[] recv_ciphertext;
    delete[] decrypted_ciphertext;
  }
}

