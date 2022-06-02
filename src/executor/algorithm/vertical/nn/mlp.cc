//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/vertical/nn/mlp.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/utils/alg/vec_util.h>
#include <falcon/utils/parser.h>
#include <falcon/algorithm/model_builder_helper.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/math/math_ops.h>

MlpModel::MlpModel() {
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_num_hidden_layers = 0;
  m_num_layers_neurons.clear();
  m_layers.clear();
}

MlpModel::MlpModel(bool with_bias, const std::vector<int>& num_layers_neurons, const std::vector<std::string>& layers_activation_funcs) {
  assert(num_layers_neurons.size() >= 2);
  assert(layers_activation_funcs.size() + 1 == num_layers_neurons.size());
  m_num_layers_neurons = num_layers_neurons;
  m_num_inputs = m_num_layers_neurons[0];
  m_num_outputs = m_num_layers_neurons[m_num_layers_neurons.size() - 1];
  m_num_hidden_layers = (int) m_num_layers_neurons.size() - 2;
  for (int i = 0; i < m_num_layers_neurons.size() - 1; i++) {
    m_layers.emplace_back(Layer(m_num_layers_neurons[i],
                                m_num_layers_neurons[i+1],
                                with_bias,
                                layers_activation_funcs[i]));
  }
}

MlpModel::MlpModel(const MlpModel &mlp_model) {
  m_num_inputs = mlp_model.m_num_inputs;
  m_num_outputs = mlp_model.m_num_outputs;
  m_num_hidden_layers = mlp_model.m_num_hidden_layers;
  m_num_layers_neurons = mlp_model.m_num_layers_neurons;
  m_layers = mlp_model.m_layers;
}

MlpModel &MlpModel::operator=(const MlpModel &mlp_model) {
  m_num_inputs = mlp_model.m_num_inputs;
  m_num_outputs = mlp_model.m_num_outputs;
  m_num_hidden_layers = mlp_model.m_num_hidden_layers;
  m_num_layers_neurons = mlp_model.m_num_layers_neurons;
  m_layers = mlp_model.m_layers;
}

MlpModel::~MlpModel() {
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_num_hidden_layers = 0;
  m_num_layers_neurons.clear();
  m_layers.clear();
}

void MlpModel::init_encrypted_weights(const Party &party, int precision) {
  log_info("Init the encrypted weights of the MLP model");
  int layer_size = (int) m_layers.size();
  for (int i = 0; i < layer_size; i++) {
    log_info("Init the encrypted weights of layer + " + std::to_string(i));
    m_layers[i].init_encrypted_weights(party, precision);
  }
}

void MlpModel::predict(const Party &party,
                       const std::vector<std::vector<double>> &predicted_samples,
                       EncodedNumber *predicted_labels) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int pred_size = (int) predicted_samples.size();
  int label_size = m_num_layers_neurons[m_num_layers_neurons.size() - 1];
  auto **predicted_labels_proba = new EncodedNumber*[pred_size];
  for (int i = 0; i < pred_size; i++) {
    predicted_labels_proba[i] = new EncodedNumber[label_size];
  }
  // call predict_proba function (another way is to implement a forward
  // computation method that returns only the final label
  predict_proba(party, predicted_samples, predicted_labels_proba);

  if (label_size == 1) {
    // directly copy
    for (int i = 0; i < pred_size; i++) {
      predicted_labels[i] = predicted_labels_proba[i][0];
    }
  } else {
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
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random, predicted_labels[i], predicted_labels[i]);
      }
      delete [] decrypted_labels_i;
    }
    broadcast_encoded_number_array(party, predicted_labels, pred_size, ACTIVE_PARTY_ID);
  }

  for (int i = 0; i < pred_size; i++) {
    delete [] predicted_labels_proba[i];
  }
  delete [] predicted_labels_proba;
  djcs_t_free_public_key(phe_pub_key);
}

void MlpModel::predict_proba(const Party &party,
                             const std::vector<std::vector<double>> &predicted_samples,
                             EncodedNumber **predicted_labels) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  int pred_size = (int) predicted_samples.size();
  std::vector<int> local_weight_sizes = sync_up_int_arr(party, party.getter_feature_num());
  int weight_size = local_weight_sizes[party.party_id];
  auto **encoded_batch_samples = new EncodedNumber*[pred_size];
  for (int i = 0; i < pred_size; i++) {
    encoded_batch_samples[i] = new EncodedNumber[weight_size];
  }

  forward_computation_fast(party, pred_size, local_weight_sizes,
                           encoded_batch_samples, predicted_labels);

  for (int i = 0; i < pred_size; i++) {
    delete [] encoded_batch_samples[i];
  }
  delete [] encoded_batch_samples;
  djcs_t_free_public_key(phe_pub_key);
}

void MlpModel::forward_computation(const Party &party,
                                   int cur_batch_size,
                                   const std::vector<int>& local_weight_sizes,
                                   EncodedNumber **encoded_batch_samples,
                                   EncodedNumber **predicted_labels,
                                   TripleDVec& layer_activation_shares,
                                   TripleDVec& layer_deriv_activation_shares) const {
  // we compute the forward computation layer-by-layer
  // for the first hidden layer, the input is the encoded_batch_samples distributed among parties
  // for the other layers, the input is the secret shares after spdz activation function
  int layer_size = (int) m_layers.size();
  // the output activation shares and derivative activation shares of each layer
  // format: (layer, idx_in_batch, neuron_idx)
  for (int l = 0; l < layer_size; l++) {
    log_info("[forward_computation]: compute layer " + std::to_string(l));
    // the first hidden layer
    if (l == 0) {
      // compute the encrypted aggregation for each neuron in layer 0
      int layer0_num_neurons = m_layers[0].m_num_neurons;
      auto** layer0_enc_outputs = new EncodedNumber*[cur_batch_size];
      for (int i = 0; i < cur_batch_size; i++) {
        layer0_enc_outputs[i] = new EncodedNumber[layer0_num_neurons];
      }

      m_layers[l].comp_1st_layer_agg_output(party, cur_batch_size,
                                            local_weight_sizes,
                                            encoded_batch_samples,
                                            layer0_num_neurons,
                                            layer0_enc_outputs);

      // next, convert to secret shares and call spdz to compute activation and deriv_activation
      int cipher_precision = std::abs(layer0_enc_outputs[0][0].getter_exponent());
      std::vector<std::vector<double>> layer0_enc_outputs_shares;
      ciphers_mat_to_secret_shares_mat(party, layer0_enc_outputs,
                                       layer0_enc_outputs_shares,
                                       cur_batch_size,
                                       layer0_num_neurons,
                                       ACTIVE_PARTY_ID,
                                       cipher_precision);

      log_info("Converted 1st layer encrypted output into secret shares.");
      std::vector<double> flatten_layer0_enc_outputs_shares = flatten_2d_vector(layer0_enc_outputs_shares);
      std::vector<int> public_values;
      public_values.push_back(falcon::ACTIVATION);
      public_values.push_back(layer0_num_neurons);
      public_values.push_back(cur_batch_size);
      falcon::SpdzMlpActivationFunc func = parse_mlp_act_func(m_layers[l].m_activation_func_str);
      public_values.push_back(func);

      falcon::SpdzMlpCompType comp_type = falcon::ACTIVATION;
      std::promise<std::vector<double>> promise_values;
      std::future<std::vector<double>> future_values = promise_values.get_future();
      std::thread spdz_pruning_check_thread(spdz_mlp_computation,
                                            party.party_num,
                                            party.party_id,
                                            party.executor_mpc_ports,
                                            party.host_names,
                                            public_values.size(),
                                            public_values,
                                            flatten_layer0_enc_outputs_shares.size(),
                                            flatten_layer0_enc_outputs_shares,
                                            comp_type,
                                            &promise_values);
      // the result values are as follows (assume public in this version):
      // best_split_index (global), best_left_impurity, and best_right_impurity
      std::vector<double> res = future_values.get();
      spdz_pruning_check_thread.join();
      log_info("[forward_computation]: communicate with spdz program finished");

      std::vector<double> res_act, res_deriv_act;
      for (int i = 0; i < flatten_layer0_enc_outputs_shares.size(); i++) {
        res_act.push_back(res[i]);
        res_deriv_act.push_back(res[i + flatten_layer0_enc_outputs_shares.size()]);
      }

      // the returned res vector is the 1d vector of secret shares of the
      // activation function and derivative of activation function for future usage
      std::vector<std::vector<double>> layer0_act_outputs_shares = expend_1d_vector(
          res_act, cur_batch_size, layer0_num_neurons);
      std::vector<std::vector<double>> layer0_deriv_act_outputs_shares = expend_1d_vector(
          res_deriv_act, cur_batch_size, layer0_num_neurons);
      layer_activation_shares.push_back(layer0_act_outputs_shares);
      layer_deriv_activation_shares.push_back(layer0_deriv_act_outputs_shares);

      for (int i = 0; i < cur_batch_size; i++) {
        delete [] layer0_enc_outputs[i];
      }
      delete [] layer0_enc_outputs;
    }

    if (l != 0) {
      // for other layers, the layer inputs are secret shares, so use another way to aggregate
      // compute the encrypted aggregation for each neuron in layer 0
      int layer_l_num_neurons = m_layers[l].m_num_neurons;
      auto** layer_l_enc_outputs = new EncodedNumber*[cur_batch_size];
      for (int i = 0; i < cur_batch_size; i++) {
        layer_l_enc_outputs[i] = new EncodedNumber[layer_l_num_neurons];
      }

      // compute the aggregation of current layer
      m_layers[l].comp_other_layer_agg_output(party, cur_batch_size,
                                              layer_activation_shares[l-1],
                                              m_layers[l].m_num_neurons,
                                              layer_l_enc_outputs);


      // next, convert to secret shares and call spdz to compute activation and deriv_activation
      int cipher_precision = std::abs(layer_l_enc_outputs[0][0].getter_exponent());
      std::vector<std::vector<double>> layer0_enc_outputs_shares;
      ciphers_mat_to_secret_shares_mat(party, layer_l_enc_outputs,
                                       layer0_enc_outputs_shares,
                                       cur_batch_size,
                                       layer_l_num_neurons,
                                       ACTIVE_PARTY_ID,
                                       cipher_precision);

      log_info("Converted 1st layer encrypted output into secret shares.");
      std::vector<double> flatten_layer0_enc_outputs_shares = flatten_2d_vector(layer0_enc_outputs_shares);
      std::vector<int> public_values;
      public_values.push_back(falcon::ACTIVATION);
      public_values.push_back(layer_l_num_neurons);
      public_values.push_back(cur_batch_size);
      falcon::SpdzMlpActivationFunc func = parse_mlp_act_func(m_layers[l].m_activation_func_str);
      public_values.push_back(func);

      falcon::SpdzMlpCompType comp_type = falcon::ACTIVATION;
      std::promise<std::vector<double>> promise_values;
      std::future<std::vector<double>> future_values = promise_values.get_future();
      std::thread spdz_pruning_check_thread(spdz_mlp_computation,
                                            party.party_num,
                                            party.party_id,
                                            party.executor_mpc_ports,
                                            party.host_names,
                                            public_values.size(),
                                            public_values,
                                            flatten_layer0_enc_outputs_shares.size(),
                                            flatten_layer0_enc_outputs_shares,
                                            comp_type,
                                            &promise_values);
      // the result values are as follows (assume public in this version):
      // best_split_index (global), best_left_impurity, and best_right_impurity
      std::vector<double> res = future_values.get();
      spdz_pruning_check_thread.join();
      log_info("[forward_computation]: communicate with spdz program finished");

      std::vector<double> res_act, res_deriv_act;
      for (int i = 0; i < flatten_layer0_enc_outputs_shares.size(); i++) {
        res_act.push_back(res[i]);
        res_deriv_act.push_back(res[i + flatten_layer0_enc_outputs_shares.size()]);
      }

      // the returned res vector is the 1d vector of secret shares of the
      // activation function and derivative of activation function for future usage
      std::vector<std::vector<double>> layer0_act_outputs_shares = expend_1d_vector(
          res_act, cur_batch_size, layer_l_num_neurons);
      std::vector<std::vector<double>> layer0_deriv_act_outputs_shares = expend_1d_vector(
          res_deriv_act, cur_batch_size, layer_l_num_neurons);
      layer_activation_shares.push_back(layer0_act_outputs_shares);
      layer_deriv_activation_shares.push_back(layer0_deriv_act_outputs_shares);

      for (int i = 0; i < cur_batch_size; i++) {
        delete [] layer_l_enc_outputs[i];
      }
      delete [] layer_l_enc_outputs;
    }

    // convert the last layer output into ciphers and assign to predicted_labels
    std::vector<std::vector<double>> batch_prediction_shares = layer_activation_shares[layer_size-1];
    for (int i = 0; i < cur_batch_size; i++) {
      secret_shares_to_ciphers(party, predicted_labels[i],
                               batch_prediction_shares[i],
                               m_num_layers_neurons[layer_size-1],
                               ACTIVE_PARTY_ID,
                               PHE_FIXED_POINT_PRECISION);
    }
    log_info("[forward_computation] finished.");
  }
}

void MlpModel::forward_computation_fast(const Party &party,
                                        int cur_batch_size,
                                        const std::vector<int> &local_weight_sizes,
                                        EncodedNumber **encoded_batch_samples,
                                        EncodedNumber **predicted_labels) const {
  // we compute the forward computation layer-by-layer
  // for the first hidden layer, the input is the encoded_batch_samples distributed among parties
  // for the other layers, the input is the secret shares after spdz activation function
  int layer_size = (int) m_layers.size();
  // the output activation shares and derivative activation shares of each layer
  // format: (layer, idx_in_batch, neuron_idx)
  std::vector<std::vector<std::vector<double>>> layer_activation_shares;
  for (int l = 0; l < layer_size; l++) {
    log_info("[forward_computation_fast]: compute layer " + std::to_string(l));
    // the first hidden layer
    if (l == 0) {
      // compute the encrypted aggregation for each neuron in layer 0
      int layer0_num_neurons = m_layers[0].m_num_neurons;
      auto** layer0_enc_outputs = new EncodedNumber*[cur_batch_size];
      for (int i = 0; i < cur_batch_size; i++) {
        layer0_enc_outputs[i] = new EncodedNumber[layer0_num_neurons];
      }

      m_layers[l].comp_1st_layer_agg_output(party, cur_batch_size,
                                            local_weight_sizes,
                                            encoded_batch_samples,
                                            layer0_num_neurons,
                                            layer0_enc_outputs);

      // next, convert to secret shares and call spdz to compute activation and deriv_activation
      int cipher_precision = std::abs(layer0_enc_outputs[0][0].getter_exponent());
      std::vector<std::vector<double>> layer0_enc_outputs_shares;
      ciphers_mat_to_secret_shares_mat(party, layer0_enc_outputs,
                                       layer0_enc_outputs_shares,
                                       cur_batch_size,
                                       layer0_num_neurons,
                                       ACTIVE_PARTY_ID,
                                       cipher_precision);

      log_info("Converted 1st layer encrypted output into secret shares.");
      std::vector<double> flatten_layer0_enc_outputs_shares = flatten_2d_vector(layer0_enc_outputs_shares);
      std::vector<int> public_values;
      public_values.push_back(falcon::ACTIVATION_FAST);
      public_values.push_back(layer0_num_neurons);
      public_values.push_back(cur_batch_size);
      falcon::SpdzMlpActivationFunc func = parse_mlp_act_func(m_layers[l].m_activation_func_str);
      public_values.push_back(func);

      falcon::SpdzMlpCompType comp_type = falcon::ACTIVATION_FAST;
      std::promise<std::vector<double>> promise_values;
      std::future<std::vector<double>> future_values = promise_values.get_future();
      std::thread spdz_pruning_check_thread(spdz_mlp_computation,
                                            party.party_num,
                                            party.party_id,
                                            party.executor_mpc_ports,
                                            party.host_names,
                                            public_values.size(),
                                            public_values,
                                            flatten_layer0_enc_outputs_shares.size(),
                                            flatten_layer0_enc_outputs_shares,
                                            comp_type,
                                            &promise_values);
      // the result values are as follows (assume public in this version):
      // best_split_index (global), best_left_impurity, and best_right_impurity
      std::vector<double> res = future_values.get();
      spdz_pruning_check_thread.join();
      log_info("[forward_computation_fast]: communicate with spdz program finished");

      // the returned res vector is the 1d vector of secret shares of the
      // activation function and derivative of activation function for future usage
      std::vector<std::vector<double>> layer0_act_outputs_shares = expend_1d_vector(
          res, cur_batch_size, layer0_num_neurons);
      layer_activation_shares.push_back(layer0_act_outputs_shares);

      for (int i = 0; i < cur_batch_size; i++) {
        delete [] layer0_enc_outputs[i];
      }
      delete [] layer0_enc_outputs;
    }

    if (l != 0) {
      // for other layers, the layer inputs are secret shares, so use another way to aggregate
      // compute the encrypted aggregation for each neuron in layer 0
      int layer_l_num_neurons = m_layers[l].m_num_neurons;
      auto** layer_l_enc_outputs = new EncodedNumber*[cur_batch_size];
      for (int i = 0; i < cur_batch_size; i++) {
        layer_l_enc_outputs[i] = new EncodedNumber[layer_l_num_neurons];
      }

      // compute the aggregation of current layer
      m_layers[l].comp_other_layer_agg_output(party, cur_batch_size,
                                              layer_activation_shares[l-1],
                                              m_layers[l].m_num_neurons,
                                              layer_l_enc_outputs);


      // next, convert to secret shares and call spdz to compute activation and deriv_activation
      int cipher_precision = std::abs(layer_l_enc_outputs[0][0].getter_exponent());
      std::vector<std::vector<double>> layer0_enc_outputs_shares;
      ciphers_mat_to_secret_shares_mat(party, layer_l_enc_outputs,
                                       layer0_enc_outputs_shares,
                                       cur_batch_size,
                                       layer_l_num_neurons,
                                       ACTIVE_PARTY_ID,
                                       cipher_precision);

      log_info("Converted 1st layer encrypted output into secret shares.");
      std::vector<double> flatten_layer0_enc_outputs_shares = flatten_2d_vector(layer0_enc_outputs_shares);
      std::vector<int> public_values;
      public_values.push_back(falcon::ACTIVATION_FAST);
      public_values.push_back(layer_l_num_neurons);
      public_values.push_back(cur_batch_size);
      falcon::SpdzMlpActivationFunc func = parse_mlp_act_func(m_layers[l].m_activation_func_str);
      public_values.push_back(func);

      falcon::SpdzMlpCompType comp_type = falcon::ACTIVATION_FAST;
      std::promise<std::vector<double>> promise_values;
      std::future<std::vector<double>> future_values = promise_values.get_future();
      std::thread spdz_pruning_check_thread(spdz_mlp_computation,
                                            party.party_num,
                                            party.party_id,
                                            party.executor_mpc_ports,
                                            party.host_names,
                                            public_values.size(),
                                            public_values,
                                            flatten_layer0_enc_outputs_shares.size(),
                                            flatten_layer0_enc_outputs_shares,
                                            comp_type,
                                            &promise_values);
      // the result values are as follows (assume public in this version):
      // best_split_index (global), best_left_impurity, and best_right_impurity
      std::vector<double> res = future_values.get();
      spdz_pruning_check_thread.join();
      log_info("[forward_computation_fast]: communicate with spdz program finished");

      // the returned res vector is the 1d vector of secret shares of the
      // activation function and derivative of activation function for future usage
      std::vector<std::vector<double>> layer0_act_outputs_shares = expend_1d_vector(
          res, cur_batch_size, layer_l_num_neurons);
      layer_activation_shares.push_back(layer0_act_outputs_shares);

      for (int i = 0; i < cur_batch_size; i++) {
        delete [] layer_l_enc_outputs[i];
      }
      delete [] layer_l_enc_outputs;
    }

    // convert the last layer output into ciphers and assign to predicted_labels
    std::vector<std::vector<double>> batch_prediction_shares = layer_activation_shares[layer_size-1];
    for (int i = 0; i < cur_batch_size; i++) {
      secret_shares_to_ciphers(party, predicted_labels[i],
                               batch_prediction_shares[i],
                               m_num_layers_neurons[layer_size-1],
                               ACTIVE_PARTY_ID,
                               PHE_FIXED_POINT_PRECISION);
    }
    log_info("[forward_computation_fast] finished.");
  }
}

void spdz_mlp_computation(int party_num,
                          int party_id,
                          std::vector<int> mpc_port_bases,
                          std::vector<std::string> party_host_names,
                          int public_value_size,
                          const std::vector<int>& public_values,
                          int private_value_size,
                          const std::vector<double>& private_values,
                          falcon::SpdzMlpCompType mlp_comp_type,
                          std::promise<std::vector<double>> *res) {
  std::vector<ssl_socket*> mpc_sockets(party_num);
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
  for (int i = 0; i < party_num; i++)
  {
    set_up_client_socket(plain_sockets[i], party_host_names[i].c_str(), mpc_port_bases[i] + i);
    send(plain_sockets[i], (octet*) &party_id, sizeof(int));
    mpc_sockets[i] = new ssl_socket(io_service, ctx, plain_sockets[i],
                                    "P" + to_string(i), "C" + to_string(party_id), true);
    if (i == 0){
      // receive gfp prime
      specification.Receive(mpc_sockets[0]);
    }
    std::cout << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_port_bases[i] + i << "." << endl;
    LOG(INFO) << "Set up socket connections for " << i << "-th spdz party succeed,"
                                                          " sockets = " << mpc_sockets[i] << ", port_num = " << mpc_port_bases[i] + i << ".";
  }
  log_info("[spdz_mlp_computation] Finish setup socket connections to spdz engines.");
  int type = specification.get<int>();
  switch (type)
  {
    case 'p':
    {
      gfp::init_field(specification.get<bigint>());
      std::cout << "Using prime " << gfp::pr() << std::endl;
      LOG(INFO) << "Using prime " << gfp::pr();
      break;
    }
    default:
      log_info("[spdz_mlp_computation] Type " + std::to_string(type) + " not implemented");
      exit(EXIT_FAILURE);
  }
  log_info("[spdz_mlp_computation] Finish initializing gfp field.");
  // std::cout << "Finish initializing gfp field." << std::endl;
  // std::cout << "batch aggregation size = " << batch_aggregation_shares.size() << std::endl;
  // send data to spdz parties
  log_info("[spdz_mlp_computation] party_id = " + std::to_string(party_id));
  if (party_id == ACTIVE_PARTY_ID) {
    // the active party sends computation id for spdz computation
    std::vector<int> computation_id;
    computation_id.push_back(mlp_comp_type);
    log_info("[spdz_mlp_computation] mlp_comp_type = " + std::to_string(mlp_comp_type));
    send_public_values(computation_id, mpc_sockets, party_num);
    // the active party sends mlp type and class num to spdz parties
    for (int i = 0; i < public_value_size; i++) {
      std::vector<int> x;
      x.push_back(public_values[i]);
      send_public_values(x, mpc_sockets, party_num);
    }
  }
  // all the parties send private shares
  log_info("[spdz_mlp_computation] private value size = " + std::to_string(private_value_size));
  for (int i = 0; i < private_value_size; i++) {
    vector<double> x;
    x.push_back(private_values[i]);
    send_private_inputs(x, mpc_sockets, party_num);
  }

  // receive result from spdz parties according to the computation type
  switch (mlp_comp_type) {
    case falcon::ACTIVATION: {
      log_info("[spdz_mlp_computation] SPDZ mlp computation pruning check returned");
      // suppose the activation and derivatives of activation shares are returned
      std::vector<double> return_values = receive_result(mpc_sockets, party_num, 2 * private_value_size);
      res->set_value(return_values);
      break;
    }
    default:
      log_info("[spdz_mlp_computation] SPDZ mlp computation type is not found.");
      exit(1);
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