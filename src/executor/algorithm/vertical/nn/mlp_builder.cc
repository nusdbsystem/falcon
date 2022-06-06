//
// Created by root on 5/21/22.
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
#include <falcon/utils/metric/regression.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>
#include <falcon/algorithm/vertical/nn/mlp_builder.h>
#include <falcon/utils/pb_converter/nn_converter.h>

#include <ctime>
#include <random>
#include <thread>
#include <future>
#include <iomanip>      // std::setprecision
#include <utility>

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>

MlpBuilder::MlpBuilder() = default;

MlpBuilder::MlpBuilder(const MlpParams &mlp_params,
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
  batch_size = mlp_params.batch_size;
  max_iteration = mlp_params.max_iteration;
  converge_threshold = mlp_params.converge_threshold;
  with_regularization = mlp_params.with_regularization;
  alpha = mlp_params.alpha;
  learning_rate = mlp_params.learning_rate;
  decay = mlp_params.decay;
  penalty = mlp_params.penalty;
  optimizer = mlp_params.optimizer;
  metric = mlp_params.metric;
  dp_budget = mlp_params.dp_budget;
  fit_bias = mlp_params.fit_bias;
  num_layers_neurons = mlp_params.num_layers_neurons;
  layers_activation_funcs = mlp_params.layers_activation_funcs;
  mlp_model = MlpModel(fit_bias, num_layers_neurons, layers_activation_funcs);
}

MlpBuilder::~MlpBuilder() {
  num_layers_neurons.clear();
  layers_activation_funcs.clear();
}

void MlpBuilder::init_encrypted_weights(const Party &party, int precision) {
  log_info("Init the encrypted weights of the MLP model");
  // if active party, init the encrypted weights and broadcast to passive parties
  std::string model_weights_str;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    mlp_model.init_encrypted_weights(party, precision);
    serialize_mlp_model(mlp_model, model_weights_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, model_weights_str);
      }
    }
  } else {
    // receive model_weights_str from active party and deserialize
    party.recv_long_message(ACTIVE_PARTY_ID, model_weights_str);
    deserialize_mlp_model(mlp_model, model_weights_str);
  }
}

void MlpBuilder::backward_computation(
    const Party &party,
    const std::vector<std::vector<double>> &batch_samples,
    EncodedNumber **predicted_labels,
    const std::vector<int> &batch_indexes,
    int precision,
    const TripleDVec& activation_shares,
    const TripleDVec& deriv_activation_shares) {
  log_info("[backward_computation] start backward computation");
  int cur_batch_size = (int) batch_samples.size();
  int n_layers = (int) mlp_model.m_layers.size();
  int n_activation_layers = (int) activation_shares.size();
  int last_layer_idx = n_layers - 1;

  // define the deltas, weight_grads, and bias_grads ciphertexts
  // deltas: (layer, sample_id, neuron)
  // weight_grads: (layer, neuron, coef)
  // bias_grads: (layer, neuron)
  std::vector<EncodedNumber**> deltas, weight_grads;
  std::vector<EncodedNumber*> bias_grads;

  // 1. compute the last layer's delta, dim = (n_samples, n_outputs)
  // if the activation function of the output layer matches the loss function
  // the deltas of the last layer is [activation_output - y] for each sample in the batch
  // [ref] https://github.com/scikit-learn/scikit-learn/issues/7122
  // note that the dimension of and the output layer shares and y is (n_samples, n_outputs)
  compute_last_layer_delta(party, predicted_labels, deltas, batch_indexes);

  // 2. compute the grads and intercepts of the last layer
  compute_loss_grad(party,
                    last_layer_idx,
                    cur_batch_size,
                    activation_shares,
                    deriv_activation_shares,
                    deltas,
                    weight_grads,
                    bias_grads);

  // 3. back-propagate until reach the first hidden layer
  for (int l_idx = last_layer_idx; l_idx > 0; l_idx--) {
    // while the rest of the hidden layers needs to multiply the derivative of activation shares
    // 3.1 update delta for (l - 1) layer
    update_layer_delta(party, l_idx,
                       cur_batch_size,
                       activation_shares,
                       deriv_activation_shares,
                       deltas);
    // 3.2 compute the coef grads and intercept grads
    compute_loss_grad(party, l_idx,
                      cur_batch_size,
                      activation_shares,
                      deriv_activation_shares,
                      deltas,
                      weight_grads,
                      bias_grads);
  }

  // 4. update the weights of each neuron in each layer
  update_encrypted_weights(party, weight_grads, bias_grads);
}

void MlpBuilder::compute_last_layer_delta(
    const Party &party,
    EncodedNumber **predicted_labels,
    std::vector<EncodedNumber **> &deltas,
    const std::vector<int> &batch_indexes) {
  log_info("[compute_last_layer_delta] computing the last layer delta");
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int prec = (int) std::abs(predicted_labels[0][0].getter_exponent());
  int cur_sample_size = (int) batch_indexes.size();
  int output_layer_size = mlp_model.m_num_outputs;
  auto** delta = new EncodedNumber*[cur_sample_size];
  for (int i = 0; i < cur_sample_size; i++) {
    delta[i] = new EncodedNumber[output_layer_size];
  }
  // active party retrieve the batch true labels
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::vector<double> plain_batch_labels;
    for (int i = 0; i < cur_sample_size; i++) {
      plain_batch_labels.push_back(training_labels[batch_indexes[i]]);
    }
    auto** batch_true_labels = new EncodedNumber*[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      batch_true_labels[i] = new EncodedNumber[output_layer_size];
    }
    get_encrypted_2d_true_labels(party, output_layer_size,
                                 plain_batch_labels, batch_true_labels, prec);
    int neg_one = -1;
    EncodedNumber enc_neg_one;
    enc_neg_one.set_integer(phe_pub_key->n[0], neg_one);
    for (int i = 0; i < cur_sample_size; i++) {
      for (int j = 0; j < output_layer_size; j++) {
        // compute [0 - y_t]
        djcs_t_aux_ep_mul(phe_pub_key,
                          batch_true_labels[i][j],
                          batch_true_labels[i][j],
                          enc_neg_one);
        // compute phe addition [pred - y_t] and assign to delta
        djcs_t_aux_ee_add(phe_pub_key, delta[i][j],
                          predicted_labels[i][j],
                          batch_true_labels[i][j]);
      }
    }

    // free memory
    for (int i = 0; i < cur_sample_size; i++) {
      delete [] batch_true_labels[i];
    }
    delete [] batch_true_labels;
  }
  broadcast_encoded_number_matrix(party, delta,
                                  cur_sample_size, output_layer_size, ACTIVE_PARTY_ID);
  deltas.push_back(delta);

  // free memory
  for (int i = 0; i < cur_sample_size; i++) {
    delete [] delta[i];
  }
  delete [] delta;
  djcs_t_free_public_key(phe_pub_key);

}

void MlpBuilder::compute_loss_grad(
    const Party &party,
    int layer_idx,
    int sample_size,
    const TripleDVec &activation_shares,
    const TripleDVec &deriv_activation_shares,
    std::vector<EncodedNumber **> &deltas,
    std::vector<EncodedNumber **> &weight_grads,
    std::vector<EncodedNumber *> &bias_grads) {
  // [ref] sklearn:
  //    coef_grads[layer] = safe_sparse_dot(activations[layer].T, deltas[layer])
  //    coef_grads[layer] += self.alpha * self.coefs_[layer]
  //    coef_grads[layer] /= n_samples
  //    intercept_grads[layer] = np.mean(deltas[layer], 0)
  // the description steps are as follows:
  //    1. compute the gradient using activation shares and delta of the layer
  //    2. add the gradient of the l2 regularization term
  //    3. divide the number of samples to get the average gradient
  //    4. compute the gradient for the bias term
  std::vector<std::vector<double>> layer_act_shares = activation_shares[layer_idx];
  auto delta = deltas[layer_idx];


  delete [] delta;
}

void MlpBuilder::update_layer_delta(
    const Party &party,
    int layer_idx,
    int sample_size,
    const TripleDVec &activation_shares,
    const TripleDVec &deriv_activation_shares,
    std::vector<EncodedNumber **> &deltas) {
  // [ref] sklearn:
  //    deltas[i - 1] = safe_sparse_dot(deltas[i], self.coefs_[i].T)
  //    inplace_derivative(activations[i], deltas[i - 1])
}

void MlpBuilder::update_encrypted_weights(
    const Party &party,
    const std::vector<EncodedNumber**>& weight_grads,
    const std::vector<EncodedNumber*>& bias_grads) {

}

void MlpBuilder::train(Party party) {
  /// The training stage consists of the following steps
  /// 1. init encrypted weights, here we assume that the parties share the encrypted weights
  /// 2. iterative computation
  ///   2.1 randomly select a batch of indexes for current iteration
  ///       2.1.1. For active party:
  ///          a) randomly select a batch of indexes
  ///          b) serialize it and send to other parties
  ///        2.1.2. For passive party:
  ///           a) receive recv_batch_indexes_str from active parties
  ///   2.2 compute layer-by-layer forward computation on the batch samples:
  ///       2.2.1. for the first hidden layer
  ///         For passive party:
  ///             a) for each neuron: compute local homomorphic aggregation based on batch samples
  ///                 and encrypted weights, and send the results to the active party
  ///         For active party:
  ///             a) receive batch_phe_aggregation string from other parties
  ///             b) add the received batch_phe_aggregation with current batch_phe_aggregation, obtaining
  ///                 [z_l] = \sum_m{[w_{lm}] * x_m} + [1] * x_0 = [x_0 + x1 * w_{l1} + ... ]
  ///                     where m is the number of neurons in the input layer, w_{lm} is the weights of neuron l
  ///                     in the hidden layer with the input layer, x_0 is the bias term of the input layer
  ///             c) since the activation function cannot compute on [z_l], convert [z_l] into secret shares
  ///                     and receive <x_l> = h([z_l]) = (<x_l>_1, ..., <x_l>_m), where m is the party number
  ///                     and each party holds <x_l>_i for i \in [1,m]
  ///        2.2.2. for the other hidden layers:
  ///             a) the parties compute: [z_j] = \sum_l{[w_{jl}] * <x_l> + b_j}
  ///             b) the parties compute <x_j> = h([z_j]) = (<x_j>_1, ..., <x_j>_m) via SPDZ
  ///        2.2.4 for the output layer:
  ///             a) the parties compute [z_i] = \sum_l{[w_{ij}] * <x_k> + b_i}
  ///             b) the parties compute <x_i> = \tau([z_i]) = (<x_i>_1, ..., <x_i>_m) via SPDZ, where \tau is the output function, e.g., softmax
  ///   2.3 compute layer-by-layer backward computation and update encrypted weights
  ///       2.3.1. for the output layer:
  ///          a) compute the loss of the batch samples: L = \sum_{b \in B} {\sum_{i} {y_i - x_i}^2} via SPDZ
  ///          b) compute the gradient of w_{ij}:
  ///                 dL/d(w_{ij}) = (d(z_i)/d(w_{ij})) * (d(x_i)/d(z_i)) * (dL/d(x_i))
  ///                              = <x_j> * h'([z_j]) * (<x_i> - <y_i>)
  ///                 where h'([z_j]) is the derivative of activation function h(.) and can be computed via SPDZ,
  ///                 and <\delta_i> = h'([z_j]) * (<x_i> - <y_i>), which will be stored for the rest of computations
  ///          c) note that if no regularization term, the encrypted weight [w_{ij}] can be updated by:
  ///                 [w_{ij}] := [w_{ij}] - \alpha [dL/d(w_{ij})]
  ///       2.3.2. for the other hidden layers:
  ///          a) compute the gradient of w_{jl}:
  ///                 dL/d(w_{jl}) = (d(z_j)/d(w_{jl})) * (d(x_j)/d(z_j)) * \sum_i {(d(z_i)/d(x_j)) * (d(x_i)/d(z_i)) * (dL/d(x_i))}
  ///                              = <x_l> * <h'([z_j])> * \sum_i {[w_{ij}] * h'([z_i] * (<x_i - y_i>)}
  ///                              = <x_l> * <h'([z_j])> * \sum_i {[w_{ij}] * <\delta_i>}
  ///          b) note that after computing the [\delta_j] = <h'([z_j])> * \sum_i {[w_{ij}] * <\delta_i>}, we can convert it into
  ///                 secret shares and store it for future usage, the secret share conversion involves the number of neurons in the hidden layer
  ///          c) similarly, if no regularization term, the encrypted weight [w_{jl}] can be updated by:
  ///                 [w_{jl}] := [w_{jl}] - \alpha [dL/d(w_{jl})]
  ///       2.3.3. for the input layer:
  ///          a) compute the gradient of w_{lm}:
  ///                 dL/d(w_{lm}) = (d(z_l)/d(w_{lm})) * (d(x_l)/d(z_l)) * \sum_j {(d(z_j)/d(x_l)) * (d(x_j)/d(z_j)) * \sum_i {(d(z_i)/d(x_j)) * (d(x_i)/d(z_i)) * (dL/d(x_i))}
  ///                              = <x_m> * <h'([z_l])> * \sum_j {[w_{jl}] * <h'([z_j])> * \sum_i {[w_{ij}] * <h'([z_i])> * (<x_i> - <y_i>) } }
  ///                              = <x_m> * <h'([z_l])> * \sum_j {[w_{jl}] * <\delta_j>}
  ///          b) note that after computing the [\delta_l] = <h'([z_l])> * \sum_j {[w_{jl}] * <\delta_j>}, we can convert it into
  ///                 secret shares and store it for future usage, the secret share conversion involves the number of neurons in the hidden layer
  ///          c) similarly, if no regularization term, the encrypted weight [w_{lm}] can be updated by:
  ///                 [w_{lm}] := [w_{lm}] - \alpha [dL/d(w_{lm})]
  /// 3. if there is regularization term, need to further think about the precision

  log_info("************* Training Start *************");

  const clock_t training_start_time = clock();

  if (optimizer != "sgd") {
    log_error("The " + optimizer + " optimizer does not supported");
    exit(EXIT_FAILURE);
  }

  // step 1: init encrypted weights (here use precision for consistence in the following)
  int n_features = party.getter_feature_num();
  std::vector<int> sync_arr = sync_up_int_arr(party, n_features);
  int encry_weights_prec = PHE_FIXED_POINT_PRECISION;
  int plain_samples_prec = PHE_FIXED_POINT_PRECISION;
  init_encrypted_weights(party, encry_weights_prec);
  log_info("[train] init encrypted MLP weights success");

  // record training data ids in data_indexes for iteratively batch selection
  std::vector<int> train_data_indexes;
  for (int i = 0; i < training_data.size(); i++) {
    train_data_indexes.push_back(i);
  }
  std::vector<std::vector<int>> batch_iter_indexes = precompute_iter_batch_idx(
      batch_size, max_iteration,train_data_indexes);

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // step 2: iteratively computation
  for (int iter = 0; iter < max_iteration; iter++) {
    log_info("-------- Iteration " + std::to_string(iter) + " --------");
    const clock_t iter_start_time = clock();

    // select batch_index
    std::vector< int> batch_indexes = sync_batch_idx(party, batch_size, batch_iter_indexes[iter]);
    log_info("-------- Iteration " + std::to_string(iter)
      + ", select_batch_idx success --------");
    // get training data with selected batch_index
    std::vector<std::vector<double> > batch_samples;
    for (int index : batch_indexes) {
      batch_samples.push_back(training_data[index]);
    }
    log_info("[train] batch_samples[0][0] = " + std::to_string(batch_samples[0][0]));

    // encode the training data
    int cur_sample_size = (int) batch_samples.size();
    auto** encoded_batch_samples = new EncodedNumber*[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_batch_samples[i] = new EncodedNumber[n_features];
    }
    encode_samples(party, batch_samples, encoded_batch_samples);
    log_info("-------- Iteration " + std::to_string(iter)
      + ", encode training data success --------");

    // forward computation for the predicted labels
    auto **predicted_labels = new EncodedNumber*[batch_indexes.size()];
    for (int i = 0; i < batch_indexes.size(); i++) {
      predicted_labels[i] = new EncodedNumber[mlp_model.m_num_outputs];
    }
    // the activation shares and derivative activation shares after forward computation
    TripleDVec layer_activation_shares, layer_deriv_activation_shares;
    int encry_agg_precision = encry_weights_prec + plain_samples_prec;
    log_info("[train] encry_agg_precision is: " + std::to_string(encry_agg_precision));
    mlp_model.forward_computation(
        party,
        cur_sample_size,
        sync_arr,
        encoded_batch_samples,
        predicted_labels,
        layer_activation_shares,
        layer_deriv_activation_shares);
    log_info("-------- Iteration " + std::to_string(iter)
      + ", forward computation success --------");
    log_info("[train] predicted_labels' precision is: "
      + std::to_string(abs(predicted_labels[0][0].getter_exponent())));

    // step 2.5: backward propagation and update weights
    backward_computation(
        party,
        batch_samples,
        predicted_labels,
        batch_indexes,
        encry_agg_precision,
        layer_activation_shares,
        layer_deriv_activation_shares);
    log_info("-------- Iteration " + std::to_string(iter)
      + ", backward computation success --------");

    const clock_t iter_finish_time = clock();
    double iter_consumed_time =
        double(iter_finish_time - iter_start_time) / CLOCKS_PER_SEC;
    log_info("-------- The " + std::to_string(iter) + "-th "
      "iteration consumed time = " + std::to_string(iter_consumed_time));

    for (int i = 0; i < cur_sample_size; i++) {
      delete [] encoded_batch_samples[i];
      delete [] predicted_labels[i];
    }
    delete [] encoded_batch_samples;
    delete [] predicted_labels;
  }

  const clock_t training_finish_time = clock();
  double training_consumed_time =
      double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("Training time = " + std::to_string(training_consumed_time));
  log_info("************* Training Finished *************");
}

void MlpBuilder::distributed_train(const Party &party, const Worker &worker) {

}

void MlpBuilder::eval(Party party, falcon::DatasetType eval_type,
                      const std::string &report_save_path) {

}

void MlpBuilder::distributed_eval(const Party &party, const Worker &worker,
                                  falcon::DatasetType eval_type) {

}