//
// Created by root on 11/11/21.
//

#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
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

LinearRegressionBuilder::LinearRegressionBuilder() = default;

LinearRegressionBuilder::LinearRegressionBuilder(LinearRegressionParams linear_reg_params,
                                                 int m_weight_size,
                                                 std::vector<std::vector<double>> m_training_data,
                                                 std::vector<std::vector<double>> m_testing_data,
                                                 std::vector<double> m_training_labels,
                                                 std::vector<double> m_testing_labels,
                                                 double m_training_accuracy,
                                                 double m_testing_accuracy)
                                                 : ModelBuilder(std::move(m_training_data),
                                                                std::move(m_testing_data),
                                                                std::move(m_training_labels),
                                                                std::move(m_testing_labels),
                                                                m_training_accuracy,
                                                                m_testing_accuracy) {
  batch_size = linear_reg_params.batch_size;
  max_iteration = linear_reg_params.max_iteration;
  converge_threshold = linear_reg_params.converge_threshold;
  with_regularization = linear_reg_params.with_regularization;
  alpha = linear_reg_params.alpha;
  learning_rate = linear_reg_params.learning_rate;
  decay = linear_reg_params.decay;
  penalty = linear_reg_params.penalty;
  optimizer = linear_reg_params.optimizer;
  metric = linear_reg_params.metric;
  dp_budget = linear_reg_params.dp_budget;
  fit_bias = linear_reg_params.fit_bias;
  linear_reg_model = LinearRegressionModel(m_weight_size);
}

LinearRegressionBuilder::~LinearRegressionBuilder() = default;

void LinearRegressionBuilder::backward_computation(const Party &party,
                                                   const std::vector<std::vector<double>> &batch_samples,
                                                   EncodedNumber *predicted_labels,
                                                   const std::vector<int> &batch_indexes,
                                                   int precision,
                                                   EncodedNumber *encrypted_gradients) {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // convert batch loss shares back to encrypted losses
  int cur_batch_size = (int) batch_indexes.size();
  auto* encrypted_batch_losses = new EncodedNumber[cur_batch_size];

  // compute the residual [f_t - y_t] of the batch samples
  // for linear regression, here precision should be
  // (local_weights.precision + sample.precision) = 4 * PHE_FIXED_POINT_PRECISION
  compute_encrypted_residual(party, batch_indexes, training_labels,
                             precision, predicted_labels, encrypted_batch_losses);

  // notice that the update formulas are different for different settings
  // (1) without regularization:
  //    [w_j] = [w_j] - lr * { 2/|B| * \sum_{i=1}^{|B|} [loss_i] * x_{ij} },
  //        where [w_j] precision is: 3 * PHE_FIXED_POINT_PRECISION
  // (2) with l2 regularization:
  //    [w_j] = [w_j] - lr * { 2/|B| * \sum_{i=1}^{|B|} [loss_i] * x_{ij} + 2 * \alpha * [w_j] },
  //        where [w_j] precision is: 3 * PHE_FIXED_POINT_PRECISION
  // (3) with l1 regularization:
  //    [w_j] = [w_j] - lr * { 2/|B| * \sum_{i=1}^{|B|} [loss_i] * x_{ij} + \alpha} (if w_j > 0)
  //    [w_j] = [w_j] - lr * { 2/|B| * \sum_{i=1}^{|B|} [loss_i] * x_{ij} - \alpha} (if w_j < 0)
  //        where [w_j] precision is: 3 * PHE_FIXED_POINT_PRECISION, and
  //        the conditional check need use spdz computation


  // after calculate loss, compute [loss_i]*x_{ij}
  auto** encoded_batch_samples = new EncodedNumber*[cur_batch_size];
  for (int i = 0; i < cur_batch_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[linear_reg_model.weight_size];
  }

  // the common item is { - lr * { 2/|B| * \sum_{i=1}^{|B|} [loss_i] * x_{ij} }
  double lr_batch = (2 * learning_rate) / this->batch_size;
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < linear_reg_model.weight_size; j++) {
      encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
                                             0 - lr_batch * batch_samples[i][j],
                                             precision);
    }
  }
  // compute common_gradients, its precision should be
  // 2 * precision = 8 * PHE_FIXED_POINT_PRECISION
  auto* common_gradients = new EncodedNumber[linear_reg_model.weight_size];
  for (int j = 0; j < linear_reg_model.weight_size; j++) {
    EncodedNumber gradient;
    // organize feature j's values of the batch
    auto* batch_feature_j = new EncodedNumber[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      batch_feature_j[i] = encoded_batch_samples[i][j];
    }
    // compute homomorphic inner product between feature j and weights
    djcs_t_aux_inner_product(phe_pub_key, party.phe_random, gradient,
                             encrypted_batch_losses, batch_feature_j,
                             cur_batch_size);
    common_gradients[j] = gradient;
    delete [] batch_feature_j;
  }

  // if no regularization
  if (!with_regularization) {
    // if no regularization, then the above common_gradients is what we desire
    for (int j = 0; j < linear_reg_model.weight_size; j++) {
      encrypted_gradients[j] = common_gradients[j];
    }
  } else {
    // if l2 regularization, the other item is { - lr * 2 * \alpha * [w_j] }
    if (penalty == "l2") {
      int common_gradients_precision = abs(common_gradients[0].getter_exponent());
      int weights_precision = abs(linear_reg_model.local_weights[0].getter_exponent());
      int plaintext_precision = common_gradients_precision - weights_precision;
      double constant = 0 - learning_rate * 2 * alpha;
      EncodedNumber encoded_constant;
      encoded_constant.set_double(phe_pub_key->n[0],
                                  constant,
                                  plaintext_precision);
      // first compute the last item of the l2 regularization
      // then, add the second item to the common_gradients
      auto* regularized_gradients = new EncodedNumber[linear_reg_model.weight_size];
      for (int j = 0; j < linear_reg_model.weight_size; j++) {
        djcs_t_aux_ep_mul(phe_pub_key,
                          regularized_gradients[j],
                          linear_reg_model.local_weights[j],
                          encoded_constant);
        djcs_t_aux_ee_add(phe_pub_key,
                          encrypted_gradients[j],
                          common_gradients[j],
                          regularized_gradients[j]);
      }
      delete [] regularized_gradients;
    }

    // if l1 regularization
    if (penalty == "l1") {

    }
  }

  // truncate the gradient precision to make sure it is consistent with [w_j]
  int dest_precision = abs(linear_reg_model.local_weights[0].getter_exponent());
  party.truncate_ciphers_precision(encrypted_gradients,
                                   cur_batch_size,
                                   ACTIVE_PARTY_ID,
                                   dest_precision);

  djcs_t_free_public_key(phe_pub_key);
  delete [] encrypted_batch_losses;
  for (int i = 0; i < cur_batch_size; i++) {
    delete [] encoded_batch_samples[i];
  }
  delete [] encoded_batch_samples;
  delete [] common_gradients;
}

void LinearRegressionBuilder::update_encrypted_weights(Party &party, EncodedNumber *encrypted_gradients) const {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // update the j-th weight in local_weight vector
  // need to make sure that the exponents of inner_product
  // and local weights are the same
  for (int j = 0; j < linear_reg_model.weight_size; j++) {
    djcs_t_aux_ee_add(phe_pub_key,
                      linear_reg_model.local_weights[j],
                      linear_reg_model.local_weights[j],
                      encrypted_gradients[j]);
  }
  djcs_t_free_public_key(phe_pub_key);
}

void LinearRegressionBuilder::train(Party party) {
  /// The training stage consists of the following steps
  /// 1. each party init encrypted local weights, "local_weights" = [w1], [w2], ...[wn]
  /// 2. iterative computation (see LogisticRegressionBuilder::train)
  ///   2.1 random select batch idx
  ///   2.2 compute batch_phe_aggregation on the batch samples [w_j * x_ij] for j = {1,...d}
  ///   2.3 compute gradient
  ///     2.3.1 if L2 regularization, do not need spdz computation,
  ///             gradient = {(1/B) \sum_{i=1}^{B} {([y_i] - [w_j * x_ij]) * 2 * x_ij}} + 2 * alpha * [w_j]
  ///     2.3.2 if L1 regularization, need spdz computation to assist
  ///             gradient = {(1/B) \sum_{i=1}^{B} {([y_i] - [w_j * x_ij]) * 2 * x_ij}} + \alpha, if w_j > 0
  ///             gradient = {(1/B) \sum_{i=1}^{B} {([y_i] - [w_j * x_ij]) * 2 * x_ij}} - \alpha, if w_j < 0
  ///           convert the [w_j] to secret shares, and return the original or negative \alpha
  ///             (need to ensure that the precision is matched during the whole computation)
  ///   2.4 use the computed gradient to update the encrypted weights
  /// 3. when the training finished, decrypt the encrypted weights if necessary

  log_info("************* Training Start *************");
  const clock_t training_start_time = clock();

  if (optimizer != "sgd") {
    log_error("The " + optimizer + " optimizer does not supported");
    exit(1);
  }

  // step 1: init encrypted local weights
  // (here use 3 * precision for consistence in the following)
  log_info("Init encrypted local weights");
  int encrypted_weights_precision = 3 * PHE_FIXED_POINT_PRECISION;
  int plaintext_samples_precision = PHE_FIXED_POINT_PRECISION;
  init_encrypted_random_numbers(party, linear_reg_model.weight_size,
                                linear_reg_model.local_weights,
                                encrypted_weights_precision);

  // record training data ids in data_indexes for iteratively batch selection
  std::vector<int> train_data_indexes;
  for (int i = 0; i < training_data.size(); i++) {
    train_data_indexes.push_back(i);
  }

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

    // encode the training data
    int cur_sample_size = (int) batch_samples.size();
    auto** encoded_batch_samples = new EncodedNumber*[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_batch_samples[i] = new EncodedNumber[linear_reg_model.weight_size];
    }
    linear_reg_model.encode_samples(party, batch_samples, encoded_batch_samples);
    log_info("-------- Iteration " + std::to_string(iter) + ", encode training data success --------");

    // compute predicted label
    auto *predicted_labels = new EncodedNumber[batch_indexes.size()];
    // 4 * fixed precision
    // weight * xj => encrypted_weights_precision + plaintext_samples_precision
    int encrypted_batch_aggregation_precision = encrypted_weights_precision + plaintext_samples_precision;
    linear_reg_model.forward_computation(
        party,
        cur_sample_size,
        encoded_batch_samples,
        encrypted_batch_aggregation_precision,
        predicted_labels);
    log_info("-------- Iteration " + std::to_string(iter) + ", forward computation success --------");

    // note: from here should be different from the logistic regression logic
    // update encrypted local weights
    auto encrypted_gradients = new EncodedNumber[linear_reg_model.weight_size];
    int update_precision = encrypted_weights_precision / 2;
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

void LinearRegressionBuilder::distributed_train(const Party &party, const Worker &worker) {

}

void LinearRegressionBuilder::eval(Party party, falcon::DatasetType eval_type, const std::string &report_save_path) {

}


// initializes the LogisticRegressionBuilder instance
// and run .train() .eval() methods, then save model
void train_linear_regression(
    Party* party,
    const std::string& params_str,
    const std::string& model_save_file,
    const std::string& model_report_file,
    int is_distributed_train, Worker* worker) {
  log_info("Run train_logistic_regression");
  log_info("is_distributed_train = " + std::to_string(is_distributed_train));

  LinearRegressionParams params;
  deserialize_lir_params(params, params_str);
  log_linear_regression_params(params);

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
    // TODO: add distributed train implementation
  }

  // weight size is different if fit_bias is true on active party
  int weight_size = party->getter_feature_num();
  double training_accuracy = 0.0;
  double testing_accuracy = 0.0;
  LinearRegressionBuilder linear_reg_builder(params,
                                             weight_size,
                                             training_data,
                                             testing_data,
                                             training_labels,
                                             testing_labels,
                                             training_accuracy,
                                             testing_accuracy);

  if (is_distributed_train == 0) {
    linear_reg_builder.train(*party);
    linear_reg_builder.eval(*party, falcon::TRAIN, model_report_file);
    linear_reg_builder.eval(*party, falcon::TEST, model_report_file);
    // save model and report
    auto* model_weights = new EncodedNumber[weight_size];
    std::string pb_lr_model_string;
    serialize_lr_model(linear_reg_builder.linear_reg_model, pb_lr_model_string);
    save_pb_model_string(pb_lr_model_string, model_save_file);
    // save_lr_model(log_reg_model_builder.log_reg_model, model_save_file);
    // save_training_report(log_reg_model.getter_training_accuracy(),
    //    log_reg_model.getter_testing_accuracy(),
    //    model_report_file);
    delete[] model_weights;
  } else {
    // TODO: add distributed train implementation
  }
}