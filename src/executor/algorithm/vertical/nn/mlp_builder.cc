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
#include <falcon/utils/alg/vec_util.h>

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
  is_classification = mlp_params.is_classification;
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
  num_layers_outputs = mlp_params.num_layers_outputs;
  layers_activation_funcs = mlp_params.layers_activation_funcs;
  mlp_model = MlpModel(is_classification, fit_bias, num_layers_outputs, layers_activation_funcs);
}

MlpBuilder::MlpBuilder(const MlpBuilder &mlp_builder) {
  training_data = mlp_builder.training_data;
  testing_data = mlp_builder.testing_data;
  training_labels = mlp_builder.training_labels;
  testing_labels = mlp_builder.testing_labels;
  training_accuracy = mlp_builder.training_accuracy;
  testing_accuracy = mlp_builder.testing_accuracy;
  is_classification = mlp_builder.is_classification;
  batch_size = mlp_builder.batch_size;
  max_iteration = mlp_builder.max_iteration;
  converge_threshold = mlp_builder.converge_threshold;
  with_regularization = mlp_builder.with_regularization;
  alpha = mlp_builder.alpha;
  learning_rate = mlp_builder.learning_rate;
  decay = mlp_builder.decay;
  penalty = mlp_builder.penalty;
  optimizer = mlp_builder.optimizer;
  metric = mlp_builder.metric;
  dp_budget = mlp_builder.dp_budget;
  fit_bias = mlp_builder.fit_bias;
  num_layers_outputs = mlp_builder.num_layers_outputs;
  layers_activation_funcs = mlp_builder.layers_activation_funcs;
  mlp_model = mlp_builder.mlp_model;
}

MlpBuilder &MlpBuilder::operator=(const MlpBuilder &mlp_builder) {
  training_data = mlp_builder.training_data;
  testing_data = mlp_builder.testing_data;
  training_labels = mlp_builder.training_labels;
  testing_labels = mlp_builder.testing_labels;
  training_accuracy = mlp_builder.training_accuracy;
  testing_accuracy = mlp_builder.testing_accuracy;
  is_classification = mlp_builder.is_classification;
  batch_size = mlp_builder.batch_size;
  max_iteration = mlp_builder.max_iteration;
  converge_threshold = mlp_builder.converge_threshold;
  with_regularization = mlp_builder.with_regularization;
  alpha = mlp_builder.alpha;
  learning_rate = mlp_builder.learning_rate;
  decay = mlp_builder.decay;
  penalty = mlp_builder.penalty;
  optimizer = mlp_builder.optimizer;
  metric = mlp_builder.metric;
  dp_budget = mlp_builder.dp_budget;
  fit_bias = mlp_builder.fit_bias;
  num_layers_outputs = mlp_builder.num_layers_outputs;
  layers_activation_funcs = mlp_builder.layers_activation_funcs;
  mlp_model = mlp_builder.mlp_model;
}

MlpBuilder::~MlpBuilder() {
  num_layers_outputs.clear();
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
    // here need to define another recv_mlp_model for deserialization because
    // the mlp_model in the mlp_builder has already allocated the memory while
    // in the nn_converter, there is another memory allocation, which will lead
    // to a deserialization problem without explicit notification.
    // CANNOT directly deserialize: deserialize_mlp_model(recv_mlp_model, model_weights_str);
    MlpModel recv_mlp_model;
    deserialize_mlp_model(recv_mlp_model, model_weights_str);
    // mlp_model = recv_mlp_model; // TODO: directly assign cause memory leak
    // assign dec_mlp_model to this->mlp_builder.mlp_model
    for (int i =0; i < mlp_model.m_layers.size(); i++) {
      log_info("[MlpParameterServer::update_encrypted_weights] enter layer " + std::to_string(i));
      int num_inputs = mlp_model.m_layers[i].m_num_inputs;
      int num_outputs = mlp_model.m_layers[i].m_num_outputs;
      for (int j = 0; j < num_inputs; j++) {
        for (int k = 0; k < num_outputs; k++) {
          mlp_model.m_layers[i].m_weight_mat[j][k] = recv_mlp_model.m_layers[i].m_weight_mat[j][k];
        }
      }
      for (int k = 0; k < num_outputs; k++) {
        mlp_model.m_layers[i].m_bias[k] = recv_mlp_model.m_layers[i].m_bias[k];
      }
    }
    log_info("[MlpBuilder::init_encrypted_weights] passive party receive and deserialize the mlp model");
  }
}

void MlpBuilder::backward_computation(
    const Party &party,
    const std::vector<std::vector<double>> &batch_samples,
    EncodedNumber **predicted_labels,
    const std::vector<int> &batch_indexes,
    const std::vector<int>& local_weight_sizes,
    int precision,
    const TripleDVec& activation_shares,
    const TripleDVec& deriv_activation_shares) {
  log_info("[backward_computation] start backward computation");
  int cur_batch_size = (int) batch_samples.size();
  int n_activation_layers = (int) activation_shares.size();
  int last_layer_idx = mlp_model.m_n_layers - 2;
  log_info("[backward_computation] cur_batch_size = " + std::to_string(cur_batch_size));
  log_info("[backward_computation] last_layer_idx = " + std::to_string(last_layer_idx));
  log_info("[backward_computation] n_activation_layers = " + std::to_string(n_activation_layers));

  // define the deltas, weight_grads, and bias_grads ciphertexts
  // deltas: (layer, sample_id, neuron)
  // weight_grads: (layer, neuron, coef)
  // bias_grads: (layer, neuron)

  // pre-allocate the memory for the deltas, weight_grads, and bias_grads
  // deltas dim: (n_layers, cur_batch_size, layer_num_outputs)
  // weight_grads dim: (n_layers, layer_num_inputs, layer_num_outputs)
  // bias_grads dim: (n_layers, layer_num_outputs)
  auto*** deltas = new EncodedNumber**[mlp_model.m_n_layers - 1];
  auto*** weight_grads = new EncodedNumber**[mlp_model.m_n_layers - 1];
  auto** bias_grads = new EncodedNumber*[mlp_model.m_n_layers - 1];
  for (int l = 0; l < mlp_model.m_n_layers - 1; l++) {
    int l_num_inputs = mlp_model.m_layers[l].m_num_inputs;
    int l_num_outputs = mlp_model.m_layers[l].m_num_outputs;
    deltas[l] = new EncodedNumber*[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      deltas[l][i] = new EncodedNumber[l_num_outputs];
    }
    weight_grads[l] = new EncodedNumber*[l_num_inputs];
    for (int i = 0; i < l_num_inputs; i++) {
      weight_grads[l][i] = new EncodedNumber[l_num_outputs];
    }
    bias_grads[l] = new EncodedNumber[l_num_outputs];
  }

  // 1. compute the last layer's delta, dim = (n_samples, n_outputs)
  // if the activation function of the output layer matches the loss function
  // the deltas of the last layer is [activation_output - y] for each sample in the batch
  // [ref] https://github.com/scikit-learn/scikit-learn/issues/7122
  // note that the dimension of and the output layer shares and y is (n_samples, n_outputs)
  compute_last_layer_delta(party, last_layer_idx,
                           predicted_labels, deltas, batch_indexes);
  log_info("[backward_computation] compute_last_layer_delta finished");

  // 2. compute the grads and intercepts of the last layer
  compute_loss_grad(party,
                    last_layer_idx,
                    cur_batch_size,
                    local_weight_sizes,
                    batch_samples,
                    activation_shares,
                    deriv_activation_shares,
                    deltas,
                    weight_grads,
                    bias_grads);

  // 3. back-propagate until reach the first hidden layer
  for (int l_idx = mlp_model.m_n_layers - 2; l_idx > 0; l_idx--) {
    // while the rest of the hidden layers needs to multiply the derivative of activation shares
    // 3.1 update delta for (l - 1) layer
    log_info("[backward_computation] l_idx = " + std::to_string(l_idx));
    update_layer_delta(party, l_idx,
                       cur_batch_size,
                       activation_shares,
                       deriv_activation_shares,
                       deltas);
    log_info("[backward_computation] update_layer_delta finished");
    // 3.2 compute the coef grads and intercept grads
    compute_loss_grad(party,
                      l_idx - 1,
                      cur_batch_size,
                      local_weight_sizes,
                      batch_samples,
                      activation_shares,
                      deriv_activation_shares,
                      deltas,
                      weight_grads,
                      bias_grads);
    log_info("[backward_computation] compute_loss_grad finished");
  }

  // 4. update the weights of each neuron in each layer
//  log_info("[backward_computation] display model before updating the weights");
//  mlp_model.display_model(party);
//  log_info("[backward_computation] display the gradients");
//  display_gradients(party, weight_grads, bias_grads);
  update_encrypted_weights(party, weight_grads, bias_grads);

//  log_info("[backward_computation] display model after updating the weights");
//  mlp_model.display_model(party);

  // free deltas, weight_grads, bias_grads memory
  for (int l = 0; l < mlp_model.m_n_layers - 1; l++) {
    delete [] bias_grads[l];
    for (int i = 0; i < cur_batch_size; i++) {
      delete [] deltas[l][i];
    }
    delete [] deltas[l];

    int layer_l_num_inputs = mlp_model.m_layers[l].m_num_inputs;
    for (int i = 0; i < layer_l_num_inputs; i++) {
      delete [] weight_grads[l][i];
    }
    delete [] weight_grads[l];
  }
  delete [] deltas;
  delete [] weight_grads;
  delete [] bias_grads;
}

void MlpBuilder::compute_last_layer_delta(
    const Party &party,
    int layer_idx,
    EncodedNumber **predicted_labels,
    EncodedNumber*** deltas,
    const std::vector<int> &batch_indexes) {
  log_info("[compute_last_layer_delta] computing the last layer delta");
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int prec = (int) std::abs(predicted_labels[0][0].getter_exponent());
  int cur_sample_size = (int) batch_indexes.size();
  int output_layer_size = mlp_model.m_num_outputs;

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
        djcs_t_aux_ee_add(phe_pub_key,
                          deltas[layer_idx][i][j],
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
  broadcast_encoded_number_matrix(party, deltas[layer_idx],
                                  cur_sample_size, output_layer_size, ACTIVE_PARTY_ID);

  log_info("[MlpBuilder::compute_last_layer_delta] layer_idx = " + std::to_string(layer_idx));
  log_info("[MlpBuilder::compute_last_layer_delta] deltas[layer_idx][0][0].prec = "
    + std::to_string(std::abs(deltas[layer_idx][0][0].getter_exponent())));

  djcs_t_free_public_key(phe_pub_key);
}

void MlpBuilder::compute_loss_grad(
    const Party &party,
    int layer_idx,
    int sample_size,
    const std::vector<int>& local_weight_sizes,
    const std::vector<std::vector<double>>& batch_samples,
    const TripleDVec &activation_shares,
    const TripleDVec &deriv_activation_shares,
    EncodedNumber*** deltas,
    EncodedNumber*** weight_grads,
    EncodedNumber** bias_grads) {
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
  log_info("[MlpBuilder::compute_loss_grad] layer_idx = " + std::to_string(layer_idx));

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int ciphers_row_size = sample_size;
  int ciphers_column_size = mlp_model.m_layers[layer_idx].m_num_outputs;
  auto** layer_delta = new EncodedNumber*[ciphers_row_size];
  for (int i = 0; i < ciphers_row_size; i++) {
    layer_delta[i] = new EncodedNumber[ciphers_column_size];
  }
  // copy layer_delta values from deltas
  for (int i = 0; i < ciphers_row_size; i++) {
    for (int j = 0; j < ciphers_column_size; j++) {
      layer_delta[i][j] = deltas[layer_idx][i][j];
    }
  }
  int shares_row_size = mlp_model.m_layers[layer_idx].m_num_inputs;
  int shares_column_size = ciphers_row_size;
  auto** layer_weight_grad = new EncodedNumber*[shares_row_size];
  for (int i = 0; i < shares_row_size; i++) {
    layer_weight_grad[i] = new EncodedNumber[ciphers_column_size];
  }

  log_info("[compute_loss_grad] shares_row_size = " + std::to_string(shares_row_size));
  log_info("[compute_loss_grad] shares_column_size = " + std::to_string(shares_column_size));
  log_info("[compute_loss_grad] ciphers_row_size = " + std::to_string(ciphers_row_size));
  log_info("[compute_loss_grad] ciphers_column_size = " + std::to_string(ciphers_column_size));
  log_info("[compute_loss_grad] init memory finished, start to compute ciphers shares matrix multiplication");

//  log_info("[compute_loss_grad] display layer_delta 1st");
//  display_encrypted_matrix(party, ciphers_row_size, ciphers_column_size, layer_delta);

  // if it is not the first hidden layer, use the cipher shares matrix multiplication
  // else, each party computes the gradients for the corresponding block
  if (layer_idx != 0) {
    std::vector<std::vector<double>> layer_act_shares = activation_shares[layer_idx];
    std::vector<std::vector<double>> layer_act_shares_trans = trans_mat(layer_act_shares);
    assert(shares_row_size == layer_act_shares_trans.size());
    assert(shares_column_size == layer_act_shares_trans[0].size());
    // can first divide n_samples (aka. sample_size)
    for (int i = 0; i < shares_row_size; i++) {
      for (int j = 0; j < shares_column_size; j++) {
        layer_act_shares_trans[i][j] = (0 - learning_rate / sample_size) * layer_act_shares_trans[i][j];
      }
    }

    cipher_shares_mat_mul(party,
                          layer_act_shares_trans,
                          layer_delta,
                          shares_row_size,
                          shares_column_size,
                          ciphers_row_size,
                          ciphers_column_size,
                          layer_weight_grad);

  } else {
    // note that each party's batch samples dimension = (n_samples, n_features)
    // while the delta dimension = (n_samples, num_outputs)
    // so we need to transpose the batch samples, and the result will be (n_features, num_outputs)
    // importantly, \sum_{i=1}^{m} (n_features_i) = num_inputs of this layer
    log_info("[compute_loss_grad] compute gradients for layer 0");
    int local_n_features = (int) batch_samples[0].size();
    int layer_output_size = mlp_model.m_layers[layer_idx].m_num_outputs;
    std::vector<std::vector<double>> trans_batch_samples = trans_mat(batch_samples);
    // multiply coefficient constant
    for (int i = 0; i < local_n_features; i++) {
      for (int j = 0; j < sample_size; j++) {
        trans_batch_samples[i][j] = (0 - learning_rate / sample_size) * trans_batch_samples[i][j];
      }
    }
    // now the dimension = (n_features, n_samples)
    auto** local_agg_mat = new EncodedNumber*[local_n_features];
    auto** encoded_trans_batch_samples = new EncodedNumber*[local_n_features];
    for (int i = 0; i < local_n_features; i++) {
      local_agg_mat[i] = new EncodedNumber[layer_output_size];
      encoded_trans_batch_samples[i] = new EncodedNumber[sample_size];
    }
    encode_samples(party, trans_batch_samples, encoded_trans_batch_samples);
    djcs_t_aux_mat_mat_ep_mult(phe_pub_key,
                               party.phe_random,
                               local_agg_mat,
                               deltas[layer_idx],
                               encoded_trans_batch_samples,
                               sample_size,
                               layer_output_size,
                               local_n_features,
                               sample_size);

    log_info("[compute_loss_grad] finish local aggregation");

    // the active party aggregate the result and broadcast
    if (party.party_type == falcon::ACTIVE_PARTY) {
      // copy self local_aggregation
      for (int i = 0; i < local_n_features; i++) {
        for (int j = 0; j < layer_output_size; j++) {
          layer_weight_grad[i][j] = local_agg_mat[i][j];
        }
      }
      // receive and aggregate
      for (int id = 0; id < party.party_num; id++) {
        if (id != party.party_id) {
          // find the index in the layer_weight_grad
          int start_idx = 0;
          for (int x = 0; x < id; x++) {
            start_idx += local_weight_sizes[x];
          }
          log_info("[compute_loss_grad] start_idx = " + std::to_string(start_idx));
          // init the matrix to receive from party id
          int party_id_weight_size = local_weight_sizes[id];
          log_info("[compute_loss_grad] party_id_weight_size = " + std::to_string(party_id_weight_size));
          log_info("[compute_loss_grad] layer_output_size = " + std::to_string(layer_output_size));

          auto** recv_local_agg_mat = new EncodedNumber*[party_id_weight_size];
          for (int i = 0; i < party_id_weight_size; i++) {
            recv_local_agg_mat[i] = new EncodedNumber[layer_output_size];
          }
          // reuse the local_mat_mul_res object to restore the received encoded matrix
          std::string recv_id_mat_str;
          party.recv_long_message(id, recv_id_mat_str);
          deserialize_encoded_number_matrix(recv_local_agg_mat, party_id_weight_size,
                                            layer_output_size, recv_id_mat_str);
          // assign to the corresponding place in layer_weight_grad
          for (int i = 0; i < party_id_weight_size; i++) {
            for (int j = 0; j < layer_output_size; j++) {
              layer_weight_grad[start_idx+i][j] = recv_local_agg_mat[i][j];
            }
          }
          for (int i = 0; i < local_weight_sizes[id]; i++) {
            delete [] recv_local_agg_mat[i];
          }
          delete [] recv_local_agg_mat;
        }
      }
    } else {
      std::string local_aggregation_mat_str;
      serialize_encoded_number_matrix(local_agg_mat, local_n_features,
                                      layer_output_size, local_aggregation_mat_str);
      party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_mat_str);
    }
    broadcast_encoded_number_matrix(party, layer_weight_grad,
                                    shares_row_size, layer_output_size, ACTIVE_PARTY_ID);

    for (int i = 0; i < local_n_features; i++) {
      delete [] local_agg_mat[i];
      delete [] encoded_trans_batch_samples[i];
    }
    delete [] local_agg_mat;
    delete [] encoded_trans_batch_samples;
  }

//  log_info("[compute_loss_grad] display layer_weight_grad 2nd");
//  display_encrypted_matrix(party, shares_row_size, ciphers_column_size, layer_weight_grad);

  log_info("[compute_loss_grad] weight gradient before regularization finished");

  if (with_regularization) {
    // only support l2 regularization currently
    if (party.party_type == falcon::ACTIVE_PARTY) {
      auto **reg_grad = new EncodedNumber *[shares_row_size];
      for (int i = 0; i < shares_row_size; i++) {
        reg_grad[i] = new EncodedNumber[ciphers_column_size];
      }
      compute_reg_grad(party, layer_idx, sample_size,
                       shares_row_size, ciphers_column_size, reg_grad);
      // aggregate the reg_grad and weight_grad
      djcs_t_aux_matrix_ele_wise_ee_add_ext(phe_pub_key,
                                            layer_weight_grad,
                                            layer_weight_grad,
                                            reg_grad,
                                            shares_row_size,
                                            ciphers_column_size);
      for (int i = 0; i < shares_row_size; i++) {
        delete[] reg_grad[i];
      }
      delete[] reg_grad;
    }
    broadcast_encoded_number_matrix(party, layer_weight_grad,
                                    shares_row_size, ciphers_column_size, ACTIVE_PARTY_ID);
  }

//  log_info("[compute_loss_grad] display layer_weight_grad 3d");
//  display_encrypted_matrix(party, shares_row_size, ciphers_column_size, layer_weight_grad);

  log_info("[compute_loss_grad] regularization term gradient finished");

  // compute the bias gradients for each neuron, which is to compute the
  // mean of the layer_delta over sample_size
  auto* layer_bias_grad = new EncodedNumber[ciphers_column_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    int layer_delta_prec = std::abs(layer_delta[0][0].getter_exponent());
    log_info("[compute_loss_grad] layer_delta_prec = " + std::to_string(layer_delta_prec));
    for (int i = 0; i < ciphers_column_size; i++) {
      layer_bias_grad[i].set_double(phe_pub_key->n[0], 0.0, layer_delta_prec);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, layer_bias_grad[i], layer_bias_grad[i]);
    }
    // aggregate the delta for each neuron in the layer
    for (int i = 0; i < ciphers_column_size; i++) {
      for (int j = 0; j < ciphers_row_size; j++) {
        djcs_t_aux_ee_add(phe_pub_key, layer_bias_grad[i],
                          layer_bias_grad[i], layer_delta[j][i]);
      }
    }
    // divide sample size to get the gradient
    double constant = (0 - learning_rate / (double) sample_size);
    EncodedNumber encoded_constant;
    encoded_constant.set_double(phe_pub_key->n[0], constant, PHE_FIXED_POINT_PRECISION);
    for (int i = 0; i < ciphers_column_size; i++) {
      djcs_t_aux_ep_mul(phe_pub_key, layer_bias_grad[i], layer_bias_grad[i], encoded_constant);
    }
  }
  broadcast_encoded_number_array(party, layer_bias_grad, ciphers_column_size, ACTIVE_PARTY_ID);

//  log_info("[compute_loss_grad] display layer_bias_grad");
//  display_encrypted_vector(party, ciphers_column_size, layer_bias_grad);

  // assign to the weight_grads and bias_grads objects
  for (int i = 0; i < shares_row_size; i++) {
    for (int j = 0; j < ciphers_column_size; j++) {
      weight_grads[layer_idx][i][j] = layer_weight_grad[i][j];
    }
  }
  for (int i = 0; i < ciphers_column_size; i++) {
    bias_grads[layer_idx][i] = layer_bias_grad[i];
  }

  // free memory
  for (int i = 0; i < shares_row_size; i++) {
    delete [] layer_weight_grad[i];
  }
  delete [] layer_weight_grad;
  delete [] layer_bias_grad;
  for (int i = 0; i < ciphers_row_size; i++) {
    delete [] layer_delta[i];
  }
  delete [] layer_delta;
  djcs_t_free_public_key(phe_pub_key);

  log_info("[MlpBuilder::compute_loss_grad] layer_idx = " + std::to_string(layer_idx));
  log_info("[MlpBuilder::compute_loss_grad] deltas[layer_idx][0][0].prec = "
               + std::to_string(std::abs(deltas[layer_idx][0][0].getter_exponent())));
}

void MlpBuilder::compute_reg_grad(const Party &party,
                                  int layer_idx,
                                  int sample_size,
                                  int row_size,
                                  int column_size,
                                  EncodedNumber **reg_grad) {
  int cur_neuron_size = (int) mlp_model.m_layers[layer_idx].m_num_outputs;
  int prev_neuron_size = (int) mlp_model.m_layers[layer_idx].m_num_inputs;
  if (prev_neuron_size != row_size || cur_neuron_size != column_size) {
    log_error("[compute_reg_grad] dimension does not match");
    exit(EXIT_FAILURE);
  }
  if (penalty != "l2") {
    log_error("[compute_reg_grad] currently only support l2 regularization");
    exit(EXIT_FAILURE);
  }

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // the regularization term is { - learning_rate * alpha * [w] / sample_size}
  double constant = 0 - (learning_rate * alpha / ((double) sample_size));
//  double constant = 0 - (learning_rate * alpha);
  EncodedNumber encoded_constant;
  encoded_constant.set_double(phe_pub_key->n[0], constant, PHE_FIXED_POINT_PRECISION);
  // copy neuron weights of the current layer, note that here is transposed
  // and multiply constant homomorphic
  for (int i = 0; i < row_size; i++) {
    for (int j = 0; j < column_size; j++) {
      reg_grad[i][j] = mlp_model.m_layers[layer_idx].m_weight_mat[i][j];
      djcs_t_aux_ep_mul(phe_pub_key, reg_grad[i][j], reg_grad[i][j], encoded_constant);
    }
  }
  log_info("[compute_reg_grad] reg_grad[0][0].precision = " + std::to_string(std::abs(reg_grad[0][0].getter_exponent())));
  log_info("[compute_reg_grad] reg_grad[0][0].type = " + std::to_string(reg_grad[0][0].getter_type()));

  djcs_t_free_public_key(phe_pub_key);
}

void MlpBuilder::update_layer_delta(
    const Party &party,
    int layer_idx,
    int sample_size,
    const TripleDVec &activation_shares,
    const TripleDVec &deriv_activation_shares,
    EncodedNumber*** deltas) {
  // [ref] sklearn:
  //    deltas[i-1] = safe_sparse_dot(deltas[i], self.coefs_[i].T)
  //    inplace_derivative(activations[i], deltas[i-1])
  // note that the delta and weights for current layer are all ciphertext,
  // need to convert delta into secret shares, and conduct ciphers and shares multiplication

  // deltas[i] dim = (sample_size, m_num_outputs[i]),
  // coefs[i] dim = (m_num_outputs[i-1], m_num_outputs[i])
  // first deltas[i-1] dim = (sample_size, m_num_outputs[i-1])
  // element-wise multiplication * activation[i-1] * deriv_activation[i-1]
  // convert delta into secret shares
  log_info("[MlpBuilder::update_layer_delta] layer_idx = " + std::to_string(layer_idx));
  int layer_weight_mat_row_size = mlp_model.m_layers[layer_idx].m_num_inputs;
  int layer_weight_mat_col_size = mlp_model.m_layers[layer_idx].m_num_outputs;
  log_info("[MlpBuilder::update_layer_delta] layer_weight_mat_row_size = " + std::to_string(layer_weight_mat_row_size));
  log_info("[MlpBuilder::update_layer_delta] layer_weight_mat_col_size = " + std::to_string(layer_weight_mat_col_size));

  auto** layer_delta = new EncodedNumber*[sample_size];
  for (int i = 0; i < sample_size; i++) {
    layer_delta[i] = new EncodedNumber[layer_weight_mat_col_size];
  }
  // copy layer_delta values from deltas
  for (int i = 0; i < sample_size; i++) {
    for (int j = 0; j < layer_weight_mat_col_size; j++) {
      layer_delta[i][j] = deltas[layer_idx][i][j];
    }
  }
  int layer_delta_prec = std::abs(layer_delta[0][0].getter_exponent());
  log_info("[MlpBuilder::update_layer_delta] cipher precision = " + std::to_string(layer_delta_prec));
  std::vector<std::vector<double>> delta_shares;
  ciphers_mat_to_secret_shares_mat(party,
                                   layer_delta,
                                   delta_shares,
                                   sample_size,
                                   layer_weight_mat_col_size,
                                   ACTIVE_PARTY_ID,
                                   layer_delta_prec);
  // transpose layer layer_idx's m_weight_mat
  auto **layer_weight_mat_trans = new EncodedNumber*[layer_weight_mat_col_size];
  for (int i = 0; i < layer_weight_mat_col_size; i++){
    layer_weight_mat_trans[i] = new EncodedNumber[layer_weight_mat_row_size];
  }
  transpose_encoded_mat(mlp_model.m_layers[layer_idx].m_weight_mat,
                        layer_weight_mat_row_size,
                        layer_weight_mat_col_size,
                        layer_weight_mat_trans);
  // init delta_prev and compute
  auto** delta_prev = new EncodedNumber*[sample_size];
  for (int i = 0; i < sample_size; i++) {
    delta_prev[i] = new EncodedNumber[layer_weight_mat_row_size];
  }
  cipher_shares_mat_mul(party,
                        delta_shares,
                        layer_weight_mat_trans,
                        sample_size,
                        layer_weight_mat_col_size,
                        layer_weight_mat_col_size,
                        layer_weight_mat_row_size,
                        delta_prev);
  // element-wise delta_prev multiplication with activation shares and deriv activation shares
  // but note that the shares are distributed on parties, it seems that previously direct
  // return the activation * derivative-activation can reduce the computation here.
  std::vector<std::vector<double>> layer_deriv_shares = deriv_activation_shares[layer_idx];
  std::vector<std::vector<double>> layer_act_shares = activation_shares[layer_idx];
  inplace_derivatives(party,
                      layer_act_shares,
                      layer_deriv_shares,
                      delta_prev,
                      sample_size,
                      layer_weight_mat_row_size);

  // assign the delta_prev
  for (int i = 0; i < sample_size; i++) {
    for (int j = 0; j < layer_weight_mat_row_size; j++) {
      deltas[layer_idx - 1][i][j] = delta_prev[i][j];
    }
  }

  for (int i = 0; i < layer_weight_mat_col_size; i++) {
    delete [] layer_weight_mat_trans[i];
  }
  delete [] layer_weight_mat_trans;
  for (int i = 0; i < sample_size; i++) {
    delete [] delta_prev[i];
  }
  delete [] delta_prev;
  for (int i = 0; i < sample_size; i++) {
    delete [] layer_delta[i];
  }
  delete [] layer_delta;
}

void MlpBuilder::inplace_derivatives(const Party &party,
                                     const std::vector<std::vector<double>> &act_shares,
                                     const std::vector<std::vector<double>> &deriv_shares,
                                     EncodedNumber **delta,
                                     int delta_row_size,
                                     int delta_col_size,
                                     int phe_precision) {
  log_info("[MlpBuilder::inplace_derivatives] update the delta ");
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int deriv_shares_row_size = (int) deriv_shares.size();
  int deriv_shares_col_size = (int) deriv_shares[0].size();
  log_info("[MlpBuilder::inplace_derivatives] deriv_shares_row_size = " + std::to_string(deriv_shares_row_size));
  log_info("[MlpBuilder::inplace_derivatives] deriv_shares_col_size = " + std::to_string(deriv_shares_col_size));

  // note that here should be element-wise multiplication, cannot directly call ciphers_shares_mat_mul function
  auto** encoded_deriv_shares = new EncodedNumber*[deriv_shares_row_size];
  for (int i = 0; i < deriv_shares_row_size; i++) {
    encoded_deriv_shares[i] = new EncodedNumber[deriv_shares_col_size];
  }
  for (int i = 0; i < deriv_shares_row_size; i++) {
    for (int j = 0; j < deriv_shares_col_size; j++) {
      encoded_deriv_shares[i][j].set_double(phe_pub_key->n[0],
                                            deriv_shares[i][j],
                                            phe_precision);
    }
  }
  djcs_t_aux_ele_wise_mat_mat_ep_mult(phe_pub_key,
                                      delta,
                                      delta,
                                      encoded_deriv_shares,
                                      delta_row_size,
                                      delta_col_size,
                                      deriv_shares_row_size,
                                      deriv_shares_col_size);
  log_info("[MlpBuilder::inplace_derivatives] party local multiplication finished");
  // active party aggregate and broadcast
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::string recv_delta_str;
        party.recv_long_message(id, recv_delta_str);
        auto** recv_delta = new EncodedNumber*[delta_row_size];
        for (int i = 0; i < delta_row_size; i++) {
          recv_delta[i] = new EncodedNumber[delta_col_size];
        }
        deserialize_encoded_number_matrix(recv_delta, delta_row_size,
                                          delta_col_size, recv_delta_str);
        djcs_t_aux_matrix_ele_wise_ee_add(
            phe_pub_key,
            delta,
            delta,
            recv_delta,
            delta_row_size,
            delta_col_size);

        for (int i = 0; i < delta_row_size; i++) {
          delete [] recv_delta[i];
        }
        delete [] recv_delta;
      }
    }
  } else {
    std::string delta_str;
    serialize_encoded_number_matrix(delta, delta_row_size, delta_col_size, delta_str);
    party.send_long_message(ACTIVE_PARTY_ID, delta_str);
  }
  broadcast_encoded_number_matrix(party, delta, delta_row_size, delta_col_size, ACTIVE_PARTY_ID);

  log_info("[MlpBuilder::inplace_derivatives] delta update finished");

  for (int i = 0; i < deriv_shares_row_size; i++) {
    delete [] encoded_deriv_shares[i];
  }
  delete [] encoded_deriv_shares;
  djcs_t_free_public_key(phe_pub_key);
}

void MlpBuilder::update_encrypted_weights(
    const Party &party,
    EncodedNumber*** weight_grads,
    EncodedNumber** bias_grads) {
  log_info("[update_encrypted_weights] start to update encrypted weights");
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
//  int layer_size = (int) mlp_model.m_layers.size();
  // update the encrypted weights and bias for each layer
  for (int i = 0; i < mlp_model.m_n_layers - 1; i++) {
    // update weight mat
    djcs_t_aux_matrix_ele_wise_ee_add_ext(phe_pub_key,
                                          mlp_model.m_layers[i].m_weight_mat,
                                          mlp_model.m_layers[i].m_weight_mat,
                                          weight_grads[i],
                                          mlp_model.m_layers[i].m_num_inputs,
                                          mlp_model.m_layers[i].m_num_outputs);
    // update bias
    for (int k = 0; k < mlp_model.m_layers[i].m_num_outputs; k++) {
      djcs_t_aux_vec_ele_wise_ee_add_ext(phe_pub_key,
                                         mlp_model.m_layers[i].m_bias,
                                         mlp_model.m_layers[i].m_bias,
                                         bias_grads[i],
                                         mlp_model.m_layers[i].m_num_outputs);
    }
  }

  // post-processing the weights to make sure that all the layers have the
  // sample precision for the encrypted weights and bias
  post_proc_model_weights(party);

  djcs_t_free_public_key(phe_pub_key);
}

void MlpBuilder::display_gradients(const Party &party, EncodedNumber ***weight_grads, EncodedNumber **bias_grads) {
  log_info("[MlpBuilder::display_gradients]");
  int n_layers = (int) mlp_model.m_layers.size();
  for (int l = 0; l < n_layers; l++) {
    log_info("[MlpBuilder::display_gradients] display layer " + std::to_string(l));
    int n_inputs_l = mlp_model.m_layers[l].m_num_inputs;
    int n_outputs_l = mlp_model.m_layers[l].m_num_outputs;
    // display m_weight_mat
    for (int i = 0; i < n_inputs_l; i++) {
      // decrypt layer l's m_weight_mat[i]
      auto* dec_weight_mat_i = new EncodedNumber[n_outputs_l];
      collaborative_decrypt(party, weight_grads[l][i],
                            dec_weight_mat_i, n_outputs_l, ACTIVE_PARTY_ID);
      if (party.party_type == falcon::ACTIVE_PARTY) {
        for (int j = 0; j < n_outputs_l; j++) {
          double w;
          dec_weight_mat_i[j].decode(w);
          log_info("[MlpBuilder::display_gradients] weight_grads["
                       + std::to_string(i) + "][" + std::to_string(j) + "] = " + std::to_string(w));
        }
      }
      delete [] dec_weight_mat_i;
    }
    // display m_bias
    auto* dec_bias_vec = new EncodedNumber[n_outputs_l];
    collaborative_decrypt(party, bias_grads[l], dec_bias_vec, n_outputs_l, ACTIVE_PARTY_ID);
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int j = 0; j < n_outputs_l; j++) {
        double b;
        dec_bias_vec[j].decode(b);
        log_info("[MlpBuilder::display_gradients] bias_grads[" + std::to_string(j) + "] = " + std::to_string(b));
      }
    }
    delete [] dec_bias_vec;
  }
}

void MlpBuilder::post_proc_model_weights(const Party& party) {
  // first, find the maximum precision of each layer (assuming that each layer has the same precision
  // second, check if the maximum precision is over a threshold, if so, truncate each layer's weights to default
  // third, else, increase the precision to maximum precision
  log_info("[post_proc_model_weights]");
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // step 1
  int max_prec = 0;
  for (int i = 0; i < mlp_model.m_layers.size(); i++) {
    int layer_weight_prec = std::abs(mlp_model.m_layers[i].m_weight_mat[0][0].getter_exponent());
    int layer_bias_prec = std::abs(mlp_model.m_layers[i].m_bias[0].getter_exponent());
    int layer_max_prec = (layer_weight_prec >= layer_bias_prec) ? layer_weight_prec : layer_bias_prec;
    if (layer_max_prec > max_prec) {
      max_prec = layer_max_prec;
    }
  }
  // step 2
  log_info("[post_proc_model_weights] max_prec = " + std::to_string(max_prec));
  if (max_prec > PHE_MAXIMUM_PRECISION) {
//    log_info("[post_proc_model_weights] display mlp model before truncation");
//    mlp_model.display_model(party);
    // truncate for each layer
    for (int i = 0; i < mlp_model.m_layers.size(); i++) {
      // truncate weight mat
      for (int j = 0; j < mlp_model.m_layers[i].m_num_inputs; j++) {
        truncate_ciphers_precision(party,
                                   mlp_model.m_layers[i].m_weight_mat[j],
                                   mlp_model.m_layers[i].m_num_outputs,
                                   ACTIVE_PARTY_ID,
                                   PHE_FIXED_POINT_PRECISION);
      }
      // truncate bias
      truncate_ciphers_precision(party,
                                 mlp_model.m_layers[i].m_bias,
                                 mlp_model.m_layers[i].m_num_outputs,
                                 ACTIVE_PARTY_ID,
                                 PHE_FIXED_POINT_PRECISION);
    }
//    log_info("[post_proc_model_weights] display mlp model after truncation");
//    mlp_model.display_model(party);
  } else {
    // increase precision for each layer
    for (int i = 0; i < mlp_model.m_layers.size(); i++) {
      // increase weight mat and bias vector
      if (party.party_type == falcon::ACTIVE_PARTY) {
        djcs_t_aux_increase_prec_mat(phe_pub_key,
                                     mlp_model.m_layers[i].m_weight_mat,
                                     max_prec,
                                     mlp_model.m_layers[i].m_weight_mat,
                                     mlp_model.m_layers[i].m_num_inputs,
                                     mlp_model.m_layers[i].m_num_outputs);
        djcs_t_aux_increase_prec_vec(phe_pub_key,
                                     mlp_model.m_layers[i].m_bias,
                                     max_prec,
                                     mlp_model.m_layers[i].m_bias,
                                     mlp_model.m_layers[i].m_num_outputs);
      }
      broadcast_encoded_number_matrix(party, mlp_model.m_layers[i].m_weight_mat,
                                      mlp_model.m_layers[i].m_num_inputs,
                                      mlp_model.m_layers[i].m_num_outputs,
                                      ACTIVE_PARTY_ID);
      broadcast_encoded_number_array(party, mlp_model.m_layers[i].m_bias,
                                     mlp_model.m_layers[i].m_num_outputs,
                                     ACTIVE_PARTY_ID);
    }
  }
  djcs_t_free_public_key(phe_pub_key);
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

  log_info("[train] m_is_classification " + std::to_string(mlp_model.m_is_classification));

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

  log_info("[train] mlp_model.m_n_layers = " + std::to_string(mlp_model.m_n_layers));

  // step 2: iteratively computation
  for (int iter = 0; iter < max_iteration; iter++) {
    log_info("-------- Iteration " + std::to_string(iter) + " --------");
    const clock_t iter_start_time = clock();

    // select batch_index
    std::vector< int> batch_indexes = sync_batch_idx(party, batch_size, batch_iter_indexes[iter]);
    log_info("-------- Iteration " + std::to_string(iter)
      + ", select_batch_idx success --------");
    for (int i = 0; i < batch_indexes.size(); i++) {
      log_info("[train] batch_indexes[" + std::to_string(i) + "] = " + std::to_string(batch_indexes[i]));
    }
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
    log_info("[train] mlp_model.m_num_outputs = " + std::to_string(mlp_model.m_num_outputs));
    // the activation shares and derivative activation shares after forward computation
    TripleDVec layer_activation_shares, layer_deriv_activation_shares;
    // push original data samples for making the computation consistent
    layer_activation_shares.push_back(batch_samples);
    layer_deriv_activation_shares.push_back(batch_samples);
    int encry_agg_precision = encry_weights_prec + plain_samples_prec;
    log_info("[train] encry_agg_precision is: " + std::to_string(encry_agg_precision));
    log_info("[train] mlp_model.m_n_layers = " + std::to_string(mlp_model.m_n_layers));
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
        sync_arr,
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

//  log_info("[train] display model weights for debug");
//  mlp_model.display_model(party);

  const clock_t training_finish_time = clock();
  double training_consumed_time =
      double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("Training time = " + std::to_string(training_consumed_time));
  log_info("************* Training Finished *************");
}

void MlpBuilder::distributed_train(const Party &party, const Worker &worker) {
  log_info("************* Distributed Training Start *************");
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
  log_info("[train] init encrypted MLP weights success");

  // required by spdz connector and mpc computation
  bigint::init_thread();

  // step 2: iteratively computation
  for (int iter = 0; iter < max_iteration; iter++) {
    log_info("-------- Iteration " + std::to_string(iter) + " --------");
    const clock_t iter_start_time = clock();
    // step 2.1 receive mlp weights from master, and assign to current mlp_model
    std::string mlp_model_str;
    worker.recv_long_message_from_ps(mlp_model_str);
    log_info("-------- Worker Iteration " + std::to_string(iter) + ", "
             "worker.receive weight success --------");
    // TODO: check why directly deserialize to mlp_model will have bugs
    MlpModel des_mlp_model;
    deserialize_mlp_model(des_mlp_model, mlp_model_str);
    // assign dec_mlp_model to mlp_model
    for (int i =0; i < mlp_model.m_layers.size(); i++) {
      int num_inputs = mlp_model.m_layers[i].m_num_inputs;
      int num_outputs = mlp_model.m_layers[i].m_num_outputs;
      for (int j = 0; j < num_inputs; j++) {
        for (int k = 0; k < num_outputs; k++) {
          mlp_model.m_layers[i].m_weight_mat[j][k] = des_mlp_model.m_layers[i].m_weight_mat[j][k];
        }
      }
      for (int k = 0; k < num_outputs; k++) {
        mlp_model.m_layers[i].m_bias[k] = des_mlp_model.m_layers[i].m_bias[k];
      }
    }

    // step 2.2 receive sample id from master for batch computation
    std::string mini_batch_indexes_str;
    worker.recv_long_message_from_ps(mini_batch_indexes_str);
    std::vector<int> mini_batch_indexes;
    deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);

    log_info("-------- "
             "Worker Iteration "
                 + std::to_string(iter)
                 + ", worker.receive sample id success, batch size "
                 + std::to_string(mini_batch_indexes.size()) + " --------");

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
    auto** encoded_batch_samples = new EncodedNumber*[cur_sample_size];
    for (int i = 0; i < cur_sample_size; i++) {
      encoded_batch_samples[i] = new EncodedNumber[n_features];
    }
    encode_samples(party, mini_batch_samples, encoded_batch_samples);
    log_info("-------- Iteration " + std::to_string(iter)
                 + ", encode training data success --------");

    // forward computation for the predicted labels
    auto **predicted_labels = new EncodedNumber*[mini_batch_indexes.size()];
    for (int i = 0; i < mini_batch_indexes.size(); i++) {
      predicted_labels[i] = new EncodedNumber[mlp_model.m_num_outputs];
    }

    // the activation shares and derivative activation shares after forward computation
    TripleDVec layer_activation_shares, layer_deriv_activation_shares;
    // push original data samples for making the computation consistent
    layer_activation_shares.push_back(mini_batch_samples);
    layer_deriv_activation_shares.push_back(mini_batch_samples);
    int encry_agg_precision = encry_weights_prec + plain_samples_prec;
    log_info("[train] encry_agg_precision is: " + std::to_string(encry_agg_precision));
    log_info("[train] mlp_model.m_n_layers = " + std::to_string(mlp_model.m_n_layers));
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

    // step 2.3: backward propagation and update weights
    backward_computation(
        party,
        mini_batch_samples,
        predicted_labels,
        mini_batch_indexes,
        sync_arr,
        encry_agg_precision,
        layer_activation_shares,
        layer_deriv_activation_shares);
    log_info("-------- Iteration " + std::to_string(iter)
                 + ", backward computation success --------");

    // step 2.4: send encrypted model to ps, ps will aggregate mlp_model
    std::string encrypted_mlp_model_str;
    serialize_mlp_model(mlp_model, encrypted_mlp_model_str);
    worker.send_long_message_to_ps(encrypted_mlp_model_str);

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

    for (int i = 0; i < cur_sample_size; i++) {
      delete [] encoded_batch_samples[i];
      delete [] predicted_labels[i];
    }
    delete [] encoded_batch_samples;
    delete [] predicted_labels;
  }

  log_info("[train] m_is_classification " + std::to_string(mlp_model.m_is_classification));

  const clock_t training_finish_time = clock();
  double training_consumed_time =
      double(training_finish_time - training_start_time) / CLOCKS_PER_SEC;
  log_info("Distributed Training time = " + std::to_string(training_consumed_time));

  log_info("************* Distributed Training Finished *************");
}

void MlpBuilder::eval(Party party, falcon::DatasetType eval_type,
                      const std::string &report_save_path) {
  std::string dataset_str = (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str + " Start *************");
  const clock_t testing_start_time = clock();
  // the testing workflow is as follows:
  //     step 1: init test data
  //     step 2: the parties call the model.predict function to compute predicted labels
  //     step 3: active party aggregates and call collaborative decryption
  //     step 4: active party computes mse metrics

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // step 1: init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ? (int) training_data.size() : (int) testing_data.size();
  std::vector<std::vector<double>> cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  log_info("[MlpBuilder::eval] dataset_size = " + std::to_string(dataset_size));

  // step 2: every party do the prediction, since all examples are required to
  // computed, there is no need communications of data index between different parties
  auto* predicted_labels = new EncodedNumber[dataset_size];
  mlp_model.predict(party, cur_test_dataset, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  log_info("[MlpBuilder::eval] start to decrypt the predicted labels");
  auto* decrypted_labels = new EncodedNumber[dataset_size];
  collaborative_decrypt(party, predicted_labels,
                        decrypted_labels,
                        dataset_size,
                        ACTIVE_PARTY_ID);

  // step 4: active party computes the metrics
  // calculate accuracy by the active party
  std::vector<double> predictions;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // decode decrypted predicted labels
    for (int i = 0; i < dataset_size; i++) {
      double x;
      decrypted_labels[i].decode(x);
      predictions.push_back(x);
    }

    // compute accuracy
    if (is_classification) {
      int correct_num = 0;
      for (int i = 0; i < dataset_size; i++) {
        log_info("[mlp_builder.eval] predictions[" + std::to_string(i) + "] = "
          + std::to_string(predictions[i]) + ", cur_test_dataset_labels["
          + std::to_string(i) + "] = " + std::to_string(cur_test_dataset_labels[i]));
         if (predictions[i] == cur_test_dataset_labels[i]) {
            correct_num += 1;
          }
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = (double) correct_num / dataset_size;
        log_info("[mlp_builder.eval] Dataset size = " + std::to_string(dataset_size)
                     + ", correct predicted num = " + std::to_string(correct_num)
                     + ", training accuracy = " + std::to_string(training_accuracy));
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = (double) correct_num / dataset_size;
        log_info("[mlp_builder.eval] Dataset size = " + std::to_string(dataset_size)
                     + ", correct predicted num = " + std::to_string(correct_num)
                     + ", testing accuracy = " + std::to_string(testing_accuracy));
      }
    } else {
      if (eval_type == falcon::TRAIN) {
        training_accuracy = mean_squared_error(predictions, cur_test_dataset_labels);
        log_info("[mlp_builder.eval] Training accuracy = " + std::to_string(training_accuracy));
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = mean_squared_error(predictions, cur_test_dataset_labels);
        log_info("[mlp_builder.eval] Testing accuracy = " + std::to_string(testing_accuracy));
      }
    }
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

void MlpBuilder::distributed_eval(const Party &party, const Worker &worker,
                                  falcon::DatasetType eval_type) {
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
  mlp_model.predict(party, cur_used_samples, predicted_labels);

  log_info("predict on mini batch samples finished");

  // step 3: active party aggregates and call collaborative decryption
  auto* decrypted_labels = new EncodedNumber[cur_used_samples_size];
  collaborative_decrypt(party, predicted_labels,
                        decrypted_labels,
                        cur_used_samples_size,
                        ACTIVE_PARTY_ID);

  // 5: ACTIVE_PARTY send to ps
  if (party.party_type == falcon::ACTIVE_PARTY) {
    std::string decrypted_predict_label_str;
    serialize_encoded_number_array(decrypted_labels, cur_used_samples_size, decrypted_predict_label_str);
    worker.send_long_message_to_ps(decrypted_predict_label_str);
  }

  log_info("decrypt the predicted labels finished");

  // 6: free resources
  delete [] predicted_labels;
  delete [] decrypted_labels;
}