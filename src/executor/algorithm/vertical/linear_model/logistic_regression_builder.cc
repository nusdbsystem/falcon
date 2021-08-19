//
// Created by wuyuncheng on 14/11/20.
//

#include <falcon/algorithm/vertical/linear_model/logistic_regression_builder.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/utils/metric/classification.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>


LogisticRegressionBuilder::LogisticRegressionBuilder() {}

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

LogisticRegressionBuilder::~LogisticRegressionBuilder() {}

void LogisticRegressionBuilder::init_encrypted_weights(const Party &party, int precision) {
  LOG(INFO) << "Init encrypted local weights.";
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  std::random_device rd;
  std::mt19937 mt(rd());
  // initialization of weights
  // random initialization with a uniform range
  std::uniform_real_distribution<double> dist(
    WEIGHTS_INIT_MIN,
    WEIGHTS_INIT_MAX
  );
  // srand(static_cast<unsigned> (time(nullptr)));
  for (int i = 0; i < log_reg_model.weight_size; i++) {
    // generate random double values within (0, 1],
    // init fixed point EncodedNumber,
    // and encrypt with public key
    double v = dist(mt);
    // double v = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    EncodedNumber t;
    t.set_double(phe_pub_key->n[0], v, precision);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, log_reg_model.local_weights[i], t);
  }

  djcs_t_free_public_key(phe_pub_key);
}

// generate a batch (array) of data indexes according to the shuffle
// array size is the batch-size
// called in each new iteration
std::vector<int> LogisticRegressionBuilder::select_batch_idx(
    const Party& party, std::vector<int> data_indexes) {
  LOG(INFO) << "LogisticRegression::select_batch_idx called\n";
  std::vector<int> batch_indexes;
  // if active party, randomly select batch indexes and send to other parties
  // if passive parties, receive batch indexes and return
  if (party.party_type == falcon::ACTIVE_PARTY) {
    LOG(INFO) << "ACTIVE_PARTY select batch indexes\n";
    // NOTE: cannot use a fixed seed here!!
    // select_batch_idx actually return a batch of indexes
    // cannot use a fixed seed, otherwise the data indexes are fixed
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(std::begin(data_indexes), std::end(data_indexes), rng);
    for (int i = 0; i < batch_size; i++) {
      // LOG(INFO) << "data_indexes selected = " << data_indexes[i] << std::endl;
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
    party.recv_long_message(ACTIVE_PARTY_ID, recv_batch_indexes_str);
    deserialize_int_array(batch_indexes, recv_batch_indexes_str);
  }
  return batch_indexes;
}

void LogisticRegressionBuilder::compute_batch_phe_aggregation(const Party &party,
    std::vector<int> batch_indexes,
    falcon::DatasetType dataset_type,
    int precision,
    EncodedNumber *batch_phe_aggregation) {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // retrieve batch samples and encode (notice to use cur_batch_size
  // instead of default batch size to avoid unexpected batch)
  int cur_batch_size = batch_indexes.size();
  std::vector<std::vector<double>> batch_samples;
  for (int i = 0; i < cur_batch_size; i++) {
    // NOTE: here is a function to calculate the inner product between
    // sample feature values and encrypted weights,
    // so needs to differentiate the dataset
    if (dataset_type == falcon::TRAIN) {
      batch_samples.push_back(training_data[batch_indexes[i]]);
    } else if (dataset_type == falcon::TEST) {
      batch_samples.push_back(testing_data[batch_indexes[i]]);
    } else {
      LOG(INFO) << "Not supported yet, reserved";
    }
  }
  EncodedNumber** encoded_batch_samples = new EncodedNumber*[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
  }
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
                                             batch_samples[i][j], precision);
    }
  }

  // compute local homomorphic aggregation
  EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
  djcs_t_aux_matrix_mult(phe_pub_key, party.phe_random,
      local_batch_phe_aggregation, log_reg_model.local_weights,
      encoded_batch_samples, cur_batch_size, log_reg_model.weight_size);

  // every party sends the local aggregation to the active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self local aggregation results
    for (int i = 0; i < cur_batch_size; i++) {
      // each element (i) in local_batch_phe_aggregation is [W].Xi,
      // Xi is a part of feature vector in example i
      batch_phe_aggregation[i] = local_batch_phe_aggregation[i];
    }

    // 1. active party receive serialized string from other parties
    // deserialize and sum to batch_phe_aggregation
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        EncodedNumber* recv_batch_phe_aggregation =
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
          djcs_t_aux_ee_add(phe_pub_key, batch_phe_aggregation[i],
                            batch_phe_aggregation[i],
                            recv_batch_phe_aggregation[i]);
        }
        delete[] recv_batch_phe_aggregation;
      }
    }

    // 2. active party serialize and send the aggregated
    // batch_phe_aggregation to other parties
    std::string global_aggregation_str;
    serialize_encoded_number_array(batch_phe_aggregation, cur_batch_size,
                                   global_aggregation_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, global_aggregation_str);
      }
    }
  } else {
    // if it's passive party, it will not do the aggregation,
    // 1. it just serialize local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_batch_phe_aggregation, cur_batch_size,
                                   local_aggregation_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);

    // 2. passive party receive the global batch aggregation
    // from the active party
    std::string recv_global_aggregation_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
    deserialize_encoded_number_array(batch_phe_aggregation, cur_batch_size,
                                     recv_global_aggregation_str);
  }

  djcs_t_free_public_key(phe_pub_key);
  for (int i = 0; i < cur_batch_size; i++) {
    delete[] encoded_batch_samples[i];
  }
  delete[] encoded_batch_samples;
  delete[] local_batch_phe_aggregation;
}

void LogisticRegressionBuilder::update_encrypted_weights(Party& party,
    std::vector<double> batch_logistic_shares,
    std::vector<double> truncated_weight_shares,
    std::vector<int> batch_indexes,
    int precision) {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // convert batch loss shares back to encrypted losses
  int cur_batch_size = batch_indexes.size();
  EncodedNumber* encrypted_batch_losses = new EncodedNumber[cur_batch_size];
  // active party receive all shares from other party and do the aggregation,
  // and board-cast it to other party.
  party.secret_shares_to_ciphers(encrypted_batch_losses, batch_logistic_shares,
                                 cur_batch_size, ACTIVE_PARTY_ID, precision);

  // compute [f_t - y_t], i.e., [batch_losses]=[batch_outputs]-[batch_labels]
  // and broadcast active party use the label to calculate loss,
  // and sent it to other parties
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::vector<double> batch_labels;
    for (int i = 0; i < cur_batch_size; i++) {
      batch_labels.push_back(training_labels[batch_indexes[i]]);
    }
    EncodedNumber* encrypted_ground_truth_labels =
        new EncodedNumber[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      // init "0-y_t"
      encrypted_ground_truth_labels[i].set_double(
          phe_pub_key->n[0], 0 - batch_labels[i], precision);
      // encrypt [0-y_t]
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         encrypted_ground_truth_labels[i],
                         encrypted_ground_truth_labels[i]);
      // compute phe addition [f_t - y_t]
      djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_losses[i],
                        encrypted_batch_losses[i],
                        encrypted_ground_truth_labels[i]);
    }
    std::string encrypted_batch_losses_str;
    serialize_encoded_number_array(encrypted_batch_losses, cur_batch_size,
                                   encrypted_batch_losses_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, encrypted_batch_losses_str);
      }
    }
    delete[] encrypted_ground_truth_labels;
    std::cout << "Finish compute encrypted loss and send to other parties"
              << std::endl;
    LOG(INFO) << "Finish compute encrypted loss and send to other parties";
  } else {
    // passive party receive encrypted loss vector
    std::string recv_encrypted_batch_losses_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_encrypted_batch_losses_str);
    deserialize_encoded_number_array(encrypted_batch_losses, cur_batch_size,
                                     recv_encrypted_batch_losses_str);
    std::cout << "Finish receive encrypted loss from the active party"
              << std::endl;
    LOG(INFO) << "Finish receive encrypted loss from the active party";
  }

  std::vector<std::vector<double>> batch_samples;
  for (int i = 0; i < cur_batch_size; i++) {
    batch_samples.push_back(training_data[batch_indexes[i]]);
  }
  EncodedNumber** encoded_batch_samples = new EncodedNumber*[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[log_reg_model.weight_size];
  }

  // update formula: [w_j]=[w_j]-lr*(1/|B|){\sum_{i=1}^{|B|} [loss_i]*x_{ij}} +
  // reg? lr*(1/|B|) is the same for all sample values, thus can be initialized
  double lr_batch = learning_rate / cur_batch_size;
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      // std::cout << "The " << i << "-th sample's " << j <<
      // "-th feature value = " << 0 - lr_batch * batch_samples[i][j] <<
      // std::endl;
      encoded_batch_samples[i][j].set_double(
          phe_pub_key->n[0], 0 - lr_batch * batch_samples[i][j], precision);
    }
  }

  // if not with_regularization, no need to convert truncated weights;
  // otherwise, need to convert truncated weights to ciphers for the update
  if (!with_regularization) {
    // update each local weight j
    for (int j = 0; j < log_reg_model.weight_size; j++) {
      EncodedNumber inner_product;
      EncodedNumber* batch_feature_j = new EncodedNumber[cur_batch_size];
      for (int i = 0; i < cur_batch_size; i++) {
        batch_feature_j[i] = encoded_batch_samples[i][j];
      }
      djcs_t_aux_inner_product(phe_pub_key, party.phe_random, inner_product,
                               encrypted_batch_losses, batch_feature_j,
                               cur_batch_size);
      // update the j-th weight in local_weight vector
      // need to make sure that the exponents of inner_product
      // and local weights are the same
      djcs_t_aux_ee_add(phe_pub_key,log_reg_model.local_weights[j],
          log_reg_model.local_weights[j], inner_product);
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

void LogisticRegressionBuilder::train(Party party) {
  std::cout << "************* Training Start *************" << std::endl;
  LOG(INFO) << "************* Training Start *************";
  const clock_t training_start_time = clock();

  /// The training stage consists of the following steps
  /// 1. init encrypted local weights, "local_weights" = [w1],[w2],...[wn]
  /// 2. iterative computation
  ///   2.1 randomly select a batch of indexes for current iteration
  ///   2.2 compute homomorphic aggregation on the batch samples:
  ///         eg, the party select m examples and have n features
  ///         "local_batch_phe_aggregation"(m*1) = { [w1]x11+[w2]x12+...+[wn]x1n,
  ///                                                [w1]x21+[w2]x22+...+[wn]x2n,
  ///                                                         ....
  ///                                                [w1]xm1+[w2]xm2+...+[wn]xmn,}
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

  if (optimizer != "sgd") {
    LOG(ERROR) << "The " << optimizer << " optimizer does not supported";
    return;
  }

  // step 1: init encrypted local weights
  // (here use 3 * precision for consistence in the following)
  int encrypted_weights_precision = 3 * PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;
  init_encrypted_weights(party, encrypted_weights_precision);

  // record training data ids in data_indexes for iteratively batch selection
  std::vector<int> data_indexes;
  for (int i = 0; i < training_data.size(); i++) {
    data_indexes.push_back(i);
  }

  google::FlushLogFiles(google::INFO);

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // step 2: iteratively computation
  for (int iter = 0; iter < max_iteration; iter++) {
    LOG(INFO) << "-------- Iteration " << iter << " --------";
    std::cout << "-------- Iteration " << iter << " --------" << std::endl;
    const clock_t iter_start_time = clock();

    // step 2.1: randomly select a batch of samples
    std::vector<int> batch_indexes = select_batch_idx(party, data_indexes);
    int cur_batch_size = batch_indexes.size();

    // std::cout << "step 2.1 success" << std::endl;

    // step 2.2: homomorphic batch aggregation
    // (the precision should be 3 * prec now)
    EncodedNumber* encrypted_batch_aggregation =
        new EncodedNumber[cur_batch_size];
    compute_batch_phe_aggregation(party, batch_indexes, falcon::TRAIN,
                                  plaintext_samples_precision,
                                  encrypted_batch_aggregation);

    // std::cout << "step 2.2 success" << std::endl;

    // step 2.3: convert the encrypted batch aggregation into secret shares
    // in order to do the exponential calculation
    int encrypted_batch_aggregation_precision =
        encrypted_weights_precision + plaintext_samples_precision;
    std::vector<double> batch_aggregation_shares;
    party.ciphers_to_secret_shares(
        encrypted_batch_aggregation, batch_aggregation_shares, cur_batch_size,
        ACTIVE_PARTY_ID, encrypted_batch_aggregation_precision);

    // std::cout << "step 2.3 success" << std::endl;

    // step 2.4: communicate with spdz parties and receive results
    // the spdz_logistic_function_computation will do the 1/(1+e^(wx)) operation
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values =
        promise_values.get_future();
    std::thread spdz_thread(spdz_logistic_function_computation, party.party_num,
                            party.party_id, SPDZ_PORT_BASE, SPDZ_PLAYER_PATH,
                            party.host_names, batch_aggregation_shares,
                            cur_batch_size, &promise_values);
    std::vector<double> batch_logistic_shares = future_values.get();
    // main thread wait spdz_thread to finish
    spdz_thread.join();

    // std::cout << "step 2.4 success" << std::endl;

    // step 2.5: update encrypted local weights
    // TODO: currently does not support with_regularization
    std::vector<double> truncated_weights_shares;
    // need to make sure that update_precision * 2 = encrypted_weights_precision
    int update_precision = encrypted_weights_precision / 2;
    update_encrypted_weights(party, batch_logistic_shares,
                             truncated_weights_shares, batch_indexes,
                             update_precision);

    // std::cout << "step 2.5 success" << std::endl;

    delete[] encrypted_batch_aggregation;

    const clock_t iter_finish_time = clock();
    double iter_consumed_time =
        double(iter_finish_time - iter_start_time) / CLOCKS_PER_SEC;
    LOG(INFO) << "-------- The " << iter
              << "-th iteration consumed time = " << iter_consumed_time
              << " --------";
    std::cout << "-------- The " << iter
              << "-th iteration consumed time = " << iter_consumed_time
              << " --------" << std::endl;

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

    google::FlushLogFiles(google::INFO);
  }

  const clock_t training_finish_time = clock();
  double training_consumed_time =
      double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Training time = " << training_consumed_time;
  LOG(INFO) << "************* Training Finished *************";
  google::FlushLogFiles(google::INFO);
}

void LogisticRegressionBuilder::eval(Party party, falcon::DatasetType eval_type,
    const std::string& report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  LOG(INFO) << "************* Evaluation on " << dataset_str << " Start *************";
  const clock_t testing_start_time = clock();

  // Classification Metrics object for performance metrics
  ClassificationMetrics ClfMetrics;

  /// the testing workflow is as follows:
  ///     step 1: init test data
  ///     step 2: the parties call the model.predict function to compute predicted labelss
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

  // step 2: every party computes partial phe summation and sends to active party
  //         now the predicted labels are computed by mpc, thus it is already the probability
  EncodedNumber* predicted_labels = new EncodedNumber[dataset_size];
  log_reg_model.predict(party, cur_test_dataset, dataset_size, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  EncodedNumber* decrypted_labels = new EncodedNumber[dataset_size];
  party.collaborative_decrypt(predicted_labels,
      decrypted_labels,
      dataset_size,
      ACTIVE_PARTY_ID);

  // std::cout << "Print predicted class" << std::endl;

  // step 4: active party computes the logistic function
  // and compare the clf metrics
  if (party.party_type == falcon::ACTIVE_PARTY) {
    if (metric == "acc") {
      // the output is a vector of integers (predicted classes)
      int pred_class;
      std::vector<int> pred_classes;

      for (int i = 0; i < dataset_size; i++) {
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
      LOG(INFO) << "Classification Confusion Matrix on " << dataset_str
                << " is:\n";
      ClfMetrics.pretty_print_cm(outfile);
      LOG(INFO) << "Classification Report on " << dataset_str << " is:\n";
      ClfMetrics.classification_report(outfile);
      outfile.close();
    } else {
      LOG(ERROR) << "The " << metric << " metric is not supported";
      return;
    }
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  delete [] predicted_labels;
  delete [] decrypted_labels;

  const clock_t testing_finish_time = clock();
  double testing_consumed_time =
      double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Evaluation time = " << testing_consumed_time;
  LOG(INFO) << "************* Evaluation on " << dataset_str
            << " Finished *************";
  google::FlushLogFiles(google::INFO);
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
  std::vector<int> indexes;
  for (int i = 0; i < dataset_size; i++) {
    indexes.push_back(i);
  }
  // homomorphic aggregation (the precision should be 4 * prec now)
  int plaintext_precision = PHE_FIXED_POINT_PRECISION;
  EncodedNumber* encrypted_aggregation = new EncodedNumber[dataset_size];
  compute_batch_phe_aggregation(party, indexes, dataset_type,
                                plaintext_precision, encrypted_aggregation);

  // step 3: active party aggregates and call collaborative decryption
  EncodedNumber* decrypted_aggregation = new EncodedNumber[dataset_size];
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

  const clock_t testing_finish_time = clock();
  double testing_consumed_time =
      double(testing_finish_time - testing_start_time) / CLOCKS_PER_SEC;
  LOG(INFO) << "Time for loss computation on " << dataset_str << " = "
            << testing_consumed_time;
  google::FlushLogFiles(google::INFO);
}

// initializes the LogisticRegressionBuilder instance
// and run .train() .eval() methods, then save model
void train_logistic_regression(Party party, std::string params_str,
                               std::string model_save_file,
                               std::string model_report_file) {
  LOG(INFO) << "Run train_logistic_regression";
  std::cout << "Run train_logistic_regression" << std::endl;

  LogisticRegressionParams params;
  deserialize_lr_params(params, params_str);

  LOG(INFO) << "params.batch_size = " << params.batch_size;
  LOG(INFO) << "params.max_iteration = " << params.max_iteration;
  LOG(INFO) << "params.converge_threshold = " << params.converge_threshold;
  LOG(INFO) << "params.with_regularization = " << params.with_regularization;
  LOG(INFO) << "params.alpha = " << params.alpha;
  LOG(INFO) << "params.learning_rate = " << params.learning_rate;
  LOG(INFO) << "params.decay = " << params.decay;
  LOG(INFO) << "params.penalty = " << params.penalty;
  LOG(INFO) << "params.optimizer = " << params.optimizer;
  LOG(INFO) << "params.multi_class = " << params.multi_class;
  LOG(INFO) << "params.metric = " << params.metric;
  LOG(INFO) << "params.dp_budget = " << params.dp_budget;
  LOG(INFO) << "params.fit_bias = " << params.fit_bias;

  // split train test data for party and populate the vectors
  std::vector<std::vector<double>> training_data;
  std::vector<std::vector<double>> testing_data;
  std::vector<double> training_labels;
  std::vector<double> testing_labels;
  double split_percentage = SPLIT_TRAIN_TEST_RATIO;
  party.split_train_test_data(split_percentage, training_data, testing_data,
                              training_labels, testing_labels);

  int weight_size = party.getter_feature_num();
  LOG(INFO) << "original weight_size = " << weight_size << std::endl;
  // retrieve the fit_bias term
  bool m_fit_bias = params.fit_bias;

  // if this is active party, and fit_bias is true
  // fit_bias or fit_intercept, for whether to plus the
  // constant _bias_ or _intercept_ term
  if ((party.party_type == falcon::ACTIVE_PARTY) && m_fit_bias) {
    LOG(INFO) << "will insert x1=1 to features\n";
    // insert x1=1 to front of the features
    double x1 = 1.0;
    for (int i = 0; i < training_data.size(); i++) {
      training_data[i].insert(training_data[i].begin(), x1);
    }
    for (int i = 0; i < testing_data.size(); i++) {
      testing_data[i].insert(testing_data[i].begin(), x1);
    }
    // update the new feature_num for the active party
    // also update the weight_size value +1
    // before passing weight_size to LogisticRegression instance below
    party.setter_feature_num(++weight_size);
    LOG(INFO) << "updated weight_size = " << weight_size << std::endl;
    LOG(INFO) << "party getter feature_num = " << party.getter_feature_num() << std::endl;
  }

  LOG(INFO) << "training_data.size() = " << training_data.size() << std::endl;
  LOG(INFO) << "training_data[0].size() = " << training_data[0].size() << std::endl;
  LOG(INFO) << "testing_data.size() = " << testing_data.size() << std::endl;
  LOG(INFO) << "testing_data[0].size() = " << testing_data[0].size() << std::endl;
  LOG(INFO) << "training_labels.size() = " << training_labels.size() << std::endl;
  LOG(INFO) << "testing_labels.size() = " << testing_labels.size() << std::endl;

  std::cout << "Init logistic regression model" << std::endl;
  LOG(INFO) << "Init logistic regression model";

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

  LOG(INFO) << "Init logistic regression model success";
  std::cout << "Init logistic regression model success" << std::endl;

  log_reg_model_builder.train(party);
  log_reg_model_builder.eval(party, falcon::TRAIN, model_report_file);
  log_reg_model_builder.eval(party, falcon::TEST, model_report_file);

  // save model and report
  std::string pb_lr_model_string;
  serialize_lr_model(log_reg_model_builder.log_reg_model, pb_lr_model_string);
  save_pb_model_string(pb_lr_model_string, model_save_file);
  // save_lr_model(log_reg_model_builder.log_reg_model, model_save_file);
  // save_training_report(log_reg_model.getter_training_accuracy(),
  //    log_reg_model.getter_testing_accuracy(),
  //    model_report_file);

  LOG(INFO) << "Trained model and report saved";
  std::cout << "Trained model and report saved" << std::endl;
  google::FlushLogFiles(google::INFO);
}

// for DEBUG
void LogisticRegressionBuilder::display_weights(Party party) {
  std::cout << "display local weights" << std::endl;
  LOG(INFO) << "display local weights";
  if (party.party_type == falcon::ACTIVE_PARTY) {
    EncodedNumber* decrypted_local_weights = new EncodedNumber[log_reg_model.weight_size];
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
    EncodedNumber* received_party_weights = new EncodedNumber[weight_num];
    EncodedNumber* decrypted_party_weights = new EncodedNumber[weight_num];
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
    EncodedNumber* decrypted_number = new EncodedNumber[1];
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
    EncodedNumber* recv_ciphertext = new EncodedNumber[1];
    EncodedNumber* decrypted_ciphertext = new EncodedNumber[1];
    deserialize_encoded_number_array(recv_ciphertext, 1, recv_ciphertext_str);
    party.collaborative_decrypt(recv_ciphertext, decrypted_ciphertext, 1,
                                ACTIVE_PARTY_ID);
    delete[] recv_ciphertext;
    delete[] decrypted_ciphertext;
  }
}