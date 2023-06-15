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
// Created by root on 5/21/22.
//

#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/algorithm/vertical/nn/mlp.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/alg/vec_util.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/utils/parser.h>

MlpModel::MlpModel() {
  m_is_classification = true;
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_num_hidden_layers = 0;
}

MlpModel::MlpModel(bool is_classification, bool with_bias,
                   const std::vector<int> &num_layers_neurons,
                   const std::vector<std::string> &layers_activation_funcs) {
  assert((layers_activation_funcs.size() + 1) == num_layers_neurons.size());
  m_is_classification = is_classification;
  m_layers_num_outputs = num_layers_neurons;
  m_num_inputs = m_layers_num_outputs[0];
  m_num_outputs = m_layers_num_outputs[m_layers_num_outputs.size() - 1];
  m_num_hidden_layers = (int)m_layers_num_outputs.size() - 2;
  log_info("[MlpModel] m_num_hidden_layers = " +
           std::to_string(m_num_hidden_layers));
  log_info("[MlpModel] m_num_outputs = " + std::to_string(m_num_outputs));
  for (int i = 0; i < m_layers_num_outputs.size() - 1; i++) {
    log_info("[MlpModel] layer " + std::to_string(i) + ": ");
    log_info("[MlpModel] m_num_inputs = " +
             std::to_string(m_layers_num_outputs[i]));
    log_info("[MlpModel] m_num_outputs = " +
             std::to_string(m_layers_num_outputs[i + 1]));
    m_layers.emplace_back(Layer(m_layers_num_outputs[i],
                                m_layers_num_outputs[i + 1], with_bias,
                                layers_activation_funcs[i]));
  }
  m_n_layers = (int)m_layers_num_outputs.size();
  log_info("[MlpModel] m_n_layers = " + std::to_string(m_n_layers));
}

MlpModel::MlpModel(const MlpModel &mlp_model) {
  m_is_classification = mlp_model.m_is_classification;
  m_num_inputs = mlp_model.m_num_inputs;
  m_num_outputs = mlp_model.m_num_outputs;
  m_num_hidden_layers = mlp_model.m_num_hidden_layers;
  m_layers_num_outputs = mlp_model.m_layers_num_outputs;
  m_layers = mlp_model.m_layers;
  m_n_layers = mlp_model.m_n_layers;
}

MlpModel &MlpModel::operator=(const MlpModel &mlp_model) {
  m_is_classification = mlp_model.m_is_classification;
  m_num_inputs = mlp_model.m_num_inputs;
  m_num_outputs = mlp_model.m_num_outputs;
  m_num_hidden_layers = mlp_model.m_num_hidden_layers;
  m_layers_num_outputs = mlp_model.m_layers_num_outputs;
  m_layers = mlp_model.m_layers;
  m_n_layers = mlp_model.m_n_layers;
}

MlpModel::~MlpModel() {
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_num_hidden_layers = 0;
}

void MlpModel::init_encrypted_weights(const Party &party, int precision) {
  log_info("Init the encrypted weights of the MLP model");
  int layer_size = (int)m_layers.size();
  log_info("[MlpModel::init_encrypted_weights] layer_size = " +
           std::to_string(layer_size));
  for (int i = 0; i < layer_size; i++) {
    log_info("Init the encrypted weights of layer + " + std::to_string(i));
    m_layers[i].init_encrypted_weights(party, precision);
  }
}

void MlpModel::predict(
    const Party &party,
    const std::vector<std::vector<double>> &predicted_samples,
    EncodedNumber *predicted_labels) const {
  log_info("[MlpModel::predict] m_is_classification " +
           std::to_string(m_is_classification));

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int pred_size = (int)predicted_samples.size();
  int label_size = m_num_outputs;
  if (m_is_classification && (m_num_outputs == 1)) {
    // binary classification
    label_size = 2;
  }
  log_info("[MlpModel::predict] label_size = " + std::to_string(label_size));
  auto **predicted_labels_proba = new EncodedNumber *[pred_size];
  for (int i = 0; i < pred_size; i++) {
    predicted_labels_proba[i] = new EncodedNumber[label_size];
  }
  // call predict_proba function (another way is to implement a forward
  // computation method that returns only the final label
  predict_proba(party, predicted_samples, predicted_labels_proba);
  log_info("[MlpModel::predict] predict_proba finished");
  log_info("[MlpModel::predict] m_is_classification " +
           std::to_string(m_is_classification));
  log_info("[MlpModel::predict] m_num_outputs = " +
           std::to_string(m_num_outputs));
  if (!m_is_classification) {
    // directly copy for regression model
    log_info("[MlpModel::predict] regression model");
    for (int i = 0; i < pred_size; i++) {
      predicted_labels[i] = predicted_labels_proba[i][0];
    }
  } else if (m_is_classification && (m_num_outputs == 1)) {
    // binary classification, check if predicted label pos >= 0.5,
    // which is the first dimension of predicted_labels_proba
    // decrypt predicted_labels_proba, find the argmax label, and encrypt
    log_info("[MlpModel::predict] begin to decrypt the labels");
    auto *encrypted_labels_pos = new EncodedNumber[pred_size];
    for (int i = 0; i < pred_size; i++) {
      encrypted_labels_pos[i] = predicted_labels_proba[i][1];
    }
    auto *decrypted_labels_pos = new EncodedNumber[pred_size];
    collaborative_decrypt(party, encrypted_labels_pos, decrypted_labels_pos,
                          pred_size, ACTIVE_PARTY_ID);
    log_info("[MlpModel::predict] re-encrypt the predicted label");
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int i = 0; i < pred_size; i++) {
        double x;
        decrypted_labels_pos[i].decode(x);
        double label = (x >= LOGREG_THRES) ? 1.0 : 0.0;
        predicted_labels[i].set_double(phe_pub_key->n[0], label);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random, predicted_labels[i],
                           predicted_labels[i]);
      }
    }
    broadcast_encoded_number_array(party, predicted_labels, pred_size,
                                   ACTIVE_PARTY_ID);
    delete[] encrypted_labels_pos;
    delete[] decrypted_labels_pos;
  } else {
    log_info("[MlpModel::predict] predict on multi-class classification");
    // decrypt predicted_labels_proba, find the argmax label, and encrypt
    for (int i = 0; i < pred_size; i++) {
      auto *decrypted_labels_i = new EncodedNumber[label_size];
      collaborative_decrypt(party, predicted_labels_proba[i],
                            decrypted_labels_i, label_size, ACTIVE_PARTY_ID);
      if (party.party_type == falcon::ACTIVE_PARTY) {
        std::vector<double> labels_i;
        for (int j = 0; j < label_size; j++) {
          double x;
          decrypted_labels_i[j].decode(x);
          labels_i.push_back(x);
        }
        double arg = argmax(labels_i);
        predicted_labels[i].set_double(phe_pub_key->n[0], arg);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random, predicted_labels[i],
                           predicted_labels[i]);
      }
      delete[] decrypted_labels_i;
    }
    broadcast_encoded_number_array(party, predicted_labels, pred_size,
                                   ACTIVE_PARTY_ID);
  }

  log_info("[MlpModel::predict] finished");

  for (int i = 0; i < pred_size; i++) {
    delete[] predicted_labels_proba[i];
  }
  delete[] predicted_labels_proba;
  djcs_t_free_public_key(phe_pub_key);
}

void MlpModel::predict_proba(
    const Party &party,
    const std::vector<std::vector<double>> &predicted_samples,
    EncodedNumber **predicted_labels) const {
  log_info("[MlpModel::predict_proba] m_is_classification " +
           std::to_string(m_is_classification));

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int pred_size = (int)predicted_samples.size();
  std::vector<int> local_weight_sizes =
      sync_up_int_arr(party, party.getter_feature_num());
  int weight_size = local_weight_sizes[party.party_id];
  auto **encoded_batch_samples = new EncodedNumber *[pred_size];
  for (int i = 0; i < pred_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[weight_size];
  }
  encode_samples(party, predicted_samples, encoded_batch_samples);

  auto **forward_predictions = new EncodedNumber *[pred_size];
  for (int i = 0; i < pred_size; i++) {
    forward_predictions[i] = new EncodedNumber[m_num_outputs];
  }

  forward_computation_fast(party, pred_size, local_weight_sizes,
                           encoded_batch_samples, forward_predictions);
  log_info("[MlpModel::predict_proba] begin to compute after forward "
           "computation fast");

  if (m_is_classification && (m_num_outputs == 1)) {
    // binary classification, the predicted labels only give the positive
    // probability as the m_num_outputs == 1
    double positive_one = 1.0;
    int negative_one = -1;
    int prediction_precision =
        std::abs(forward_predictions[0][0].getter_exponent());
    auto *predicted_labels_neg = new EncodedNumber[pred_size];
    if (party.party_type == falcon::ACTIVE_PARTY) {
      EncodedNumber pos_one_double, neg_one_int;
      pos_one_double.set_double(phe_pub_key->n[0], positive_one,
                                prediction_precision);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, pos_one_double,
                         pos_one_double);
      neg_one_int.set_integer(phe_pub_key->n[0], negative_one);
      for (int i = 0; i < pred_size; i++) {
        EncodedNumber label_neg;
        djcs_t_aux_ep_mul(phe_pub_key, label_neg, forward_predictions[i][0],
                          neg_one_int);
        djcs_t_aux_ee_add(phe_pub_key, label_neg, label_neg, pos_one_double);
        predicted_labels_neg[i] = label_neg;
      }
    }
    broadcast_encoded_number_array(party, predicted_labels_neg, pred_size,
                                   ACTIVE_PARTY_ID);
    log_info("[MlpModel::predict_proba] finish broadcasting the "
             "predicted_labels_neg");
    // assign the negative probability to the
    for (int i = 0; i < pred_size; i++) {
      predicted_labels[i][0] = predicted_labels_neg[i];
      predicted_labels[i][1] = forward_predictions[i][0];
    }
    delete[] predicted_labels_neg;
  } else {
    for (int i = 0; i < pred_size; i++) {
      for (int j = 0; j < m_num_outputs; j++) {
        predicted_labels[i][j] = forward_predictions[i][j];
      }
    }
  }

  log_info("[MlpModel::predict_proba] finish predict_proba");

  for (int i = 0; i < pred_size; i++) {
    delete[] encoded_batch_samples[i];
    delete[] forward_predictions[i];
  }
  delete[] encoded_batch_samples;
  delete[] forward_predictions;
  djcs_t_free_public_key(phe_pub_key);
}

void MlpModel::forward_computation(const Party &party, int cur_batch_size,
                                   const std::vector<int> &local_weight_sizes,
                                   EncodedNumber **encoded_batch_samples,
                                   EncodedNumber **predicted_labels,
                                   TripleDVec &activation_shares,
                                   TripleDVec &deriv_activation_shares) const {
  // we compute the forward computation layer-by-layer
  // for the first hidden layer, the input is the encoded_batch_samples
  // distributed among parties for the other layers, the input is the secret
  // shares after spdz activation function

  // the output activation shares and derivative activation shares of each layer
  // format: (layer, idx_in_batch, neuron_idx)
  for (int l_idx = 0; l_idx < m_n_layers - 1; l_idx++) {
    log_info("---------- [forward_computation]: compute layer " +
             std::to_string(l_idx) + "-----------");
    struct timespec forward_layer_start;
    clock_gettime(CLOCK_MONOTONIC, &forward_layer_start);

    //    log_info("[forward_computation] display m_weight_mat");
    //    display_encrypted_matrix(party, m_layers[l_idx].m_num_inputs,
    //    m_layers[l_idx].m_num_outputs, m_layers[l_idx].m_weight_mat);
    //    log_info("[forward_computation] display m_bias");
    //    display_encrypted_vector(party, m_layers[l_idx].m_num_outputs,
    //    m_layers[l_idx].m_bias);

    int cur_layer_num_outputs = m_layers[l_idx].m_num_outputs;
    log_info("[forward_computation]: cur_layer_num_outputs = " +
             std::to_string(cur_layer_num_outputs));
    auto **cur_layer_enc_outputs = new EncodedNumber *[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      cur_layer_enc_outputs[i] = new EncodedNumber[cur_layer_num_outputs];
    }
    // the first hidden layer
    if (l_idx == 0) {
      // compute the encrypted aggregation for each neuron in the first hidden
      // layer
      m_layers[l_idx].comp_1st_layer_agg_output(
          party, cur_batch_size, local_weight_sizes, encoded_batch_samples,
          cur_layer_num_outputs, cur_layer_enc_outputs);

      //      log_info("[forward_computation] display cur_layer_enc_outputs for
      //      debug"); display_encrypted_matrix(party, cur_batch_size,
      //      cur_layer_num_outputs, cur_layer_enc_outputs);
    } else {
      // for other layers, the layer inputs are secret shares, so use another
      // way to aggregate compute the encrypted aggregation for each neuron in
      // layer 0 compute the aggregation of current layer
      log_info("[forward_computation] activation_shares.size = " +
               std::to_string(activation_shares.size()));
      m_layers[l_idx].comp_other_layer_agg_output(
          party, cur_batch_size, activation_shares[l_idx],
          cur_layer_num_outputs, cur_layer_enc_outputs);

      //      log_info("[forward_computation] display cur_layer_enc_outputs for
      //      debug"); display_encrypted_matrix(party, cur_batch_size,
      //      cur_layer_num_outputs, cur_layer_enc_outputs);
    }

    // next, convert to secret shares and call spdz to compute activation and
    // deriv_activation
    int cipher_precision =
        std::abs(cur_layer_enc_outputs[0][0].getter_exponent());
    std::vector<std::vector<double>> cur_layer_enc_outputs_shares;
    ciphers_mat_to_secret_shares_mat(party, cur_layer_enc_outputs,
                                     cur_layer_enc_outputs_shares,
                                     cur_batch_size, cur_layer_num_outputs,
                                     ACTIVE_PARTY_ID, cipher_precision);
    log_info("[forward_computation] converted 1st hidden layer "
             "encrypted output into secret shares.");
    std::vector<double> flatten_layer_enc_outputs_shares =
        flatten_2d_vector(cur_layer_enc_outputs_shares);
    std::vector<int> public_values;
    public_values.push_back(cur_batch_size);
    public_values.push_back(cur_layer_num_outputs);
    falcon::SpdzMlpActivationFunc func =
        parse_mlp_act_func(m_layers[l_idx].m_activation_func_str);
    public_values.push_back(func);

    falcon::SpdzMlpCompType comp_type = falcon::ACTIVATION;
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values =
        promise_values.get_future();
    std::thread spdz_pruning_check_thread(
        spdz_mlp_computation, party.party_num, party.party_id,
        party.executor_mpc_ports, party.host_names, public_values.size(),
        public_values, flatten_layer_enc_outputs_shares.size(),
        flatten_layer_enc_outputs_shares, comp_type, &promise_values);
    // the result values are as follows (assume to be private, i.e., secret
    // shares):
    std::vector<double> spdz_res = future_values.get();
    spdz_pruning_check_thread.join();
    log_info("[forward_computation]: communicate with spdz finished");
    log_info("[forward_computation]: spdz_res.size() = " +
             std::to_string(spdz_res.size()));
    std::vector<std::vector<double>> layer_act_outputs_shares,
        layer_deriv_act_outputs_shares;
    parse_spdz_act_comp_res(spdz_res, cur_batch_size, cur_layer_num_outputs,
                            layer_act_outputs_shares,
                            layer_deriv_act_outputs_shares);
    log_info("[forward_computation] layer_act_outputs_shares.row_size = " +
             std::to_string(layer_act_outputs_shares.size()));
    log_info("[forward_computation] layer_act_outputs_shares.col_size = " +
             std::to_string(layer_act_outputs_shares[0].size()));
    log_info(
        "[forward_computation] layer_deriv_act_outputs_shares.row_size = " +
        std::to_string(layer_deriv_act_outputs_shares.size()));
    log_info(
        "[forward_computation] layer_deriv_act_outputs_shares.col_size = " +
        std::to_string(layer_deriv_act_outputs_shares[0].size()));

    activation_shares.push_back(layer_act_outputs_shares);
    deriv_activation_shares.push_back(layer_deriv_act_outputs_shares);

    //    log_info("[forward_computation] display activation shares for layer "
    //    + std::to_string(l_idx)); display_shares_matrix(party,
    //    layer_act_outputs_shares);

    struct timespec forward_layer_finish;
    clock_gettime(CLOCK_MONOTONIC, &forward_layer_finish);
    double layer_consumed_time =
        (double)(forward_layer_finish.tv_sec - forward_layer_start.tv_sec);
    layer_consumed_time +=
        (double)(forward_layer_finish.tv_nsec - forward_layer_start.tv_nsec) /
        1000000000.0;
    log_info("-------- The " + std::to_string(l_idx) +
             "-th "
             "layer forward consumed time = " +
             std::to_string(layer_consumed_time));

    for (int i = 0; i < cur_batch_size; i++) {
      delete[] cur_layer_enc_outputs[i];
    }
    delete[] cur_layer_enc_outputs;
  }
  // convert the last layer output into ciphers and assign to predicted_labels
  std::vector<std::vector<double>> batch_prediction_shares =
      activation_shares[activation_shares.size() - 1];
  log_info("[forward_computation] activation_shares.size() - 1 = " +
           std::to_string(activation_shares.size() - 1));
  log_info("[forward_computation] batch_prediction_shares.size = " +
           std::to_string(batch_prediction_shares.size()));
  log_info("[forward_computation] batch_prediction_shares[0].size = " +
           std::to_string(batch_prediction_shares[0].size()));

  for (int i = 0; i < cur_batch_size; i++) {
    secret_shares_to_ciphers(party, predicted_labels[i],
                             batch_prediction_shares[i], m_num_outputs,
                             ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  }

  //  log_info("[forward_computation] display predicted_labels for debug");
  //  display_encrypted_matrix(party, cur_batch_size, m_num_outputs,
  //  predicted_labels);

  log_info("[forward_computation] finished.");
}

void MlpModel::parse_spdz_act_comp_res(
    const std::vector<double> &res, int dim1_size, int dim2_size,
    std::vector<std::vector<double>> &act_outputs_shares,
    std::vector<std::vector<double>> &deriv_act_outputs_shares) {
  log_info("[parse_spdz_act_comp_res] parse the received shares from spdz in "
           "forward computation");
  int flatten_size = dim1_size * dim2_size;
  std::vector<double> res_act, res_deriv_act;
  for (int i = 0; i < flatten_size; i++) {
    res_act.push_back(res[i]);
    res_deriv_act.push_back(res[i + flatten_size]);
  }
  // the returned res vector is the 1d vector of secret shares of the
  // activation function and derivative of activation function for future usage
  act_outputs_shares = expend_1d_vector(res_act, dim1_size, dim2_size);
  deriv_act_outputs_shares =
      expend_1d_vector(res_deriv_act, dim1_size, dim2_size);
}

void MlpModel::forward_computation_fast(
    const Party &party, int cur_batch_size,
    const std::vector<int> &local_weight_sizes,
    EncodedNumber **encoded_batch_samples,
    EncodedNumber **predicted_labels) const {
  // we compute the forward computation layer-by-layer
  // for the first hidden layer, the input is the encoded_batch_samples
  // distributed among parties for the other layers, the input is the secret
  // shares after spdz activation function the output activation shares and
  // derivative activation shares of each layer format: (layer, idx_in_batch,
  // neuron_idx)
  TripleDVec activation_shares;
  std::vector<std::vector<double>> dummy;
  activation_shares.push_back(dummy);
  for (int l_idx = 0; l_idx < m_n_layers - 1; l_idx++) {
    log_info("[forward_computation_fast]: compute layer " +
             std::to_string(l_idx));
    int cur_layer_num_outputs = m_layers[l_idx].m_num_outputs;
    // record the layer encrypted output in cur_layer_enc_outputs, dim =
    // (cur_batch_size, cur_layer_num_outputs)
    auto **cur_layer_enc_outputs = new EncodedNumber *[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      cur_layer_enc_outputs[i] = new EncodedNumber[cur_layer_num_outputs];
    }
    // the first hidden layer
    if (l_idx == 0) {
      // compute the encrypted aggregation for each neuron in the first hidden
      // layer
      m_layers[l_idx].comp_1st_layer_agg_output(
          party, cur_batch_size, local_weight_sizes, encoded_batch_samples,
          cur_layer_num_outputs, cur_layer_enc_outputs);
    } else {
      // for other layers, the layer inputs are secret shares, so use another
      // way to aggregate compute the encrypted aggregation for each neuron in
      // layer 0 compute the aggregation of current layer
      m_layers[l_idx].comp_other_layer_agg_output(
          party, cur_batch_size, activation_shares[l_idx],
          cur_layer_num_outputs, cur_layer_enc_outputs);
    }

    // next, convert to secret shares and call spdz to compute activation and
    // deriv_activation
    int cipher_precision =
        std::abs(cur_layer_enc_outputs[0][0].getter_exponent());
    std::vector<std::vector<double>> cur_layer_enc_outputs_shares;
    ciphers_mat_to_secret_shares_mat(party, cur_layer_enc_outputs,
                                     cur_layer_enc_outputs_shares,
                                     cur_batch_size, cur_layer_num_outputs,
                                     ACTIVE_PARTY_ID, cipher_precision);

    //    log_info("[forward_computation_fast] display input secret shares to
    //    spdz"); display_shares_matrix(party, cur_layer_enc_outputs_shares);

    log_info("[forward_computation_fast] converted 1st hidden layer "
             "encrypted output into secret shares.");
    std::vector<double> flatten_layer_enc_outputs_shares =
        flatten_2d_vector(cur_layer_enc_outputs_shares);
    std::vector<int> public_values;
    public_values.push_back(cur_batch_size);
    public_values.push_back(cur_layer_num_outputs);
    falcon::SpdzMlpActivationFunc func =
        parse_mlp_act_func(m_layers[l_idx].m_activation_func_str);
    public_values.push_back(func);

    falcon::SpdzMlpCompType comp_type = falcon::ACTIVATION_FAST;
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values =
        promise_values.get_future();
    std::thread spdz_pruning_check_thread(
        spdz_mlp_computation, party.party_num, party.party_id,
        party.executor_mpc_ports, party.host_names, public_values.size(),
        public_values, flatten_layer_enc_outputs_shares.size(),
        flatten_layer_enc_outputs_shares, comp_type, &promise_values);
    // the result values are as follows (assume to be private, i.e., secret
    // shares):
    std::vector<double> res = future_values.get();
    spdz_pruning_check_thread.join();
    log_info("[forward_computation_fast]: communicate with spdz finished");

    //    for (int i = 0; i < flatten_layer_enc_outputs_shares.size(); i++) {
    //      log_info("[forward_computation_fast] res[" + std::to_string(i) + "]
    //      = " + std::to_string(res[i]));
    //    }

    // the returned res vector is the 1d vector of secret shares of the
    // activation function and derivative of activation function for future
    // usage
    std::vector<std::vector<double>> layer_act_outputs_shares =
        expend_1d_vector(res, cur_batch_size, cur_layer_num_outputs);
    activation_shares.push_back(layer_act_outputs_shares);

    for (int i = 0; i < cur_batch_size; i++) {
      delete[] cur_layer_enc_outputs[i];
    }
    delete[] cur_layer_enc_outputs;
  }

  // convert the last layer output into ciphers and assign to predicted_labels
  std::vector<std::vector<double>> batch_prediction_shares =
      activation_shares[activation_shares.size() - 1];

  //  for (int i = 0; i < batch_prediction_shares.size(); i++) {
  //    for (int j = 0; j < batch_prediction_shares[0].size(); j++) {
  //      log_info("[forward_computation_fast] batch_prediction_shares[" +
  //      std::to_string(i)
  //        + "][" + std::to_string(j) + "] = " +
  //        std::to_string(batch_prediction_shares[i][j]));
  //    }
  //  }

  //  log_info("[forward_computation_fast] display batch_prediction_shares");
  //  display_shares_matrix(party, batch_prediction_shares);

  for (int i = 0; i < cur_batch_size; i++) {
    secret_shares_to_ciphers(party, predicted_labels[i],
                             batch_prediction_shares[i], m_num_outputs,
                             ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);
  }

  //  log_info("[forward_computation_fast] display predicted_labels for debug");
  //  display_encrypted_matrix(party, cur_batch_size, m_num_outputs,
  //  predicted_labels);

  log_info("[forward_computation_fast] finished.");
}

void MlpModel::display_model(const Party &party) {
  log_info("------------ [MlpModel::display_model] ------------");
  int n_layers = (int)m_layers.size();
  for (int l = 0; l < n_layers; l++) {
    log_info("[MlpModel::display_model] display layer " + std::to_string(l));
    int n_inputs_l = m_layers[l].m_num_inputs;
    int n_outputs_l = m_layers[l].m_num_outputs;
    // display m_weight_mat
    for (int i = 0; i < n_inputs_l; i++) {
      // decrypt layer l's m_weight_mat[i]
      auto *dec_weight_mat_i = new EncodedNumber[n_outputs_l];
      collaborative_decrypt(party, m_layers[l].m_weight_mat[i],
                            dec_weight_mat_i, n_outputs_l, ACTIVE_PARTY_ID);
      if (party.party_type == falcon::ACTIVE_PARTY) {
        for (int j = 0; j < n_outputs_l; j++) {
          double w;
          dec_weight_mat_i[j].decode(w);
          log_info("[MlpModel::display_model] m_weight_mat[" +
                   std::to_string(i) + "][" + std::to_string(j) +
                   "] = " + std::to_string(w));
        }
      }
      delete[] dec_weight_mat_i;
    }
    // display m_bias
    auto *dec_bias_vec = new EncodedNumber[n_outputs_l];
    collaborative_decrypt(party, m_layers[l].m_bias, dec_bias_vec, n_outputs_l,
                          ACTIVE_PARTY_ID);
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int j = 0; j < n_outputs_l; j++) {
        double b;
        dec_bias_vec[j].decode(b);
        log_info("[MlpModel::display_model] m_bias[" + std::to_string(j) +
                 "] = " + std::to_string(b));
      }
    }
    delete[] dec_bias_vec;
  }
}

void spdz_mlp_computation(int party_num, int party_id,
                          std::vector<int> mpc_port_bases,
                          std::vector<std::string> party_host_names,
                          int public_value_size,
                          const std::vector<int> &public_values,
                          int private_value_size,
                          const std::vector<double> &private_values,
                          falcon::SpdzMlpCompType mlp_comp_type,
                          std::promise<std::vector<double>> *res) {
  std::vector<ssl_socket *> mpc_sockets(party_num);
  //  setup_sockets(party_num,
  //                party_id,
  //                std::move(mpc_player_path),
  //                std::move(party_host_names),
  //                mpc_port_base,
  //                mpc_sockets);
  // Here put the whole setup socket code together, as using a function call
  // would result in a problem when deleting the created sockets
  // setup connections from this party to each spdz party socket
  vector<int> plain_sockets(party_num);
  // ssl_ctx ctx(mpc_player_path, "C" + to_string(party_id));
  ssl_ctx ctx("C" + to_string(party_id));
  // std::cout << "correct init ctx" << std::endl;
  ssl_service io_service;
  octetStream specification;
  log_info("[spdz_mlp_computation] begin connect to spdz parties");
  log_info("[spdz_mlp_computation] party_num = " + std::to_string(party_num));
  for (int i = 0; i < party_num; i++) {
    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(),
                         mpc_port_bases[i] + i);
    send(plain_sockets[i], (octet *)&party_id, sizeof(int));
    mpc_sockets[i] =
        new ssl_socket(io_service, ctx, plain_sockets[i], "P" + to_string(i),
                       "C" + to_string(party_id), true);
    if (i == 0) {
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    std::cout << "Set up socket connections for " << i
              << "-th spdz party succeed,"
                 " sockets = "
              << mpc_sockets[i] << ", port_num = " << mpc_port_bases[i] + i
              << "." << endl;
    LOG(INFO) << "Set up socket connections for " << i
              << "-th spdz party succeed,"
                 " sockets = "
              << mpc_sockets[i] << ", port_num = " << mpc_port_bases[i] + i
              << ".";
  }
  log_info("[spdz_mlp_computation] Finish setup socket connections to spdz "
           "engines.");
  int type = specification.get<int>();
  switch (type) {
  case 'p': {
    gfp::init_field(specification.get<bigint>());
    std::cout << "Using prime " << gfp::pr() << std::endl;
    LOG(INFO) << "Using prime " << gfp::pr();
    break;
  }
  default:
    log_info("[spdz_mlp_computation] Type " + std::to_string(type) +
             " not implemented");
    exit(EXIT_FAILURE);
  }
  log_info("[spdz_mlp_computation] Finish initializing gfp field.");
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size()
  // << std::endl; send data to spdz parties
  log_info("[spdz_mlp_computation] party_id = " + std::to_string(party_id));
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(mlp_comp_type);
    log_info("[spdz_mlp_computation] mlp_comp_type = " +
             std::to_string(mlp_comp_type));
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends mlp type and class num to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  // all the parties send private shares
  log_info("[spdz_mlp_computation] private value size = " +
           std::to_string(private_value_size));
  for (int i = 0; i < private_value_size; i++) {
    vector<double> x;
    x.push_back(private_values[i]);
    send_private_inputs(x, mpc_sockets, party_num);
  }

  // receive result from spdz parties according to the computation type
  switch (mlp_comp_type) {
  case falcon::ACTIVATION: {
    log_info("[spdz_mlp_computation] SPDZ mlp computation activation returned");
    // suppose the activation and derivatives of activation shares are returned
    std::vector<double> return_values =
        receive_result(mpc_sockets, party_num, 2 * private_value_size);
    res->set_value(return_values);
    break;
  }
  case falcon::ACTIVATION_FAST: {
    log_info(
        "[spdz_mlp_computation] SPDZ mlp computation activation fast returned");
    // suppose the activation shares are returned
    std::vector<double> return_values =
        receive_result(mpc_sockets, party_num, private_value_size);
    res->set_value(return_values);
    break;
  }
  default:
    log_info("[spdz_mlp_computation] SPDZ mlp computation type is not found.");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < party_num; i++) {
    close_client_socket(plain_sockets[i]);
  }

  // free memory and close mpc_sockets
  for (int i = 0; i < party_num; i++) {
    delete mpc_sockets[i];
    mpc_sockets[i] = nullptr;
  }
}