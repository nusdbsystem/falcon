//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/vertical/nn/layer.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/algorithm/model_builder_helper.h>

Layer::Layer() {
  m_num_neurons = 0;
  m_num_inputs_per_neuron = 0;
  m_neurons.clear();
}

Layer::Layer(int num_neurons, int num_inputs_per_neuron,
             bool with_bias, const std::string &activation_func_str) {
  m_num_neurons = num_neurons;
  m_num_inputs_per_neuron = num_inputs_per_neuron;
  m_activation_func_str = activation_func_str;
  for (int i = 0; i < num_neurons; i++) {
    m_neurons.emplace_back(Neuron(num_inputs_per_neuron, with_bias));
  }
}

Layer::Layer(const Layer &layer) {
  m_num_neurons = layer.m_num_neurons;
  m_num_inputs_per_neuron = layer.m_num_inputs_per_neuron;
  m_activation_func_str = layer.m_activation_func_str;
  m_neurons = layer.m_neurons;
}

Layer &Layer::operator=(const Layer &layer) {
  m_num_neurons = layer.m_num_neurons;
  m_num_inputs_per_neuron = layer.m_num_inputs_per_neuron;
  m_activation_func_str = layer.m_activation_func_str;
  m_neurons = layer.m_neurons;
}

Layer::~Layer() {
  m_num_neurons = 0;
  m_num_inputs_per_neuron = 0;
  m_neurons.clear();
}

void Layer::init_encrypted_weights(const Party &party, int precision) {
  // according to sklearn init coef function, here
  // set factor=6.0 if the activation function is not 'logistic' or 'sigmoid'
  // otherwise set factor=2.0
  double factor = 6.0;
  if (m_activation_func_str == "sigmoid") {
    factor = 2.0;
  }
  // according to sklearn, limit = sqrt(factor / (num_inputs + num_outputs))
  double limit = sqrt(factor / (m_num_inputs_per_neuron + m_num_neurons));
  int neuron_size = (int) m_neurons.size();
  for (int i = 0; i < neuron_size; i++) {
    m_neurons[i].init_encrypted_weights(party, limit, precision);
  }
}

void Layer::comp_1st_layer_agg_output(const Party &party,
                                      int cur_batch_size,
                                      const std::vector<int>& local_weight_sizes,
                                      EncodedNumber **encoded_batch_samples,
                                      int output_size,
                                      EncodedNumber **res) const {
  log_info("[comp_1st_layer_agg_output] compute the encrypted aggregation for the first hidden layer");
  int neuron_size = (int) m_neurons.size();
  if (neuron_size != output_size) {
    log_error("[comp_1st_layer_agg_output] neuron size does not match");
    exit(EXIT_FAILURE);
  }
  int precision = std::abs(encoded_batch_samples[0][0].getter_exponent())
      + std::abs(m_neurons[0].m_weights[0].getter_exponent());

  // transform matrix helper
  auto **res_helper = new EncodedNumber*[neuron_size];
  for (int i = 0; i < neuron_size; i++) {
    res_helper[i] = new EncodedNumber[cur_batch_size];
  }

  for (int i = 0; i < neuron_size; i++) {
    m_neurons[i].compute_batch_phe_aggregation(
        party, cur_batch_size,
        local_weight_sizes, encoded_batch_samples,
        precision, res_helper[i]);
  }

  // convert res_helper into res
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < neuron_size; j++) {
      res[i][j] = res_helper[j][i];
    }
  }

  // free memory
  for (int i = 0; i < neuron_size; i++) {
    delete [] res_helper[i];
  }
  delete [] res_helper;
}

void Layer::comp_other_layer_agg_output(const Party &party,
                                        int cur_batch_size,
                                        const std::vector<std::vector<double>> &prev_layer_outputs_shares,
                                        int output_size,
                                        EncodedNumber **res) const {
  // execution method:
  // 1. since the inputs are secret shares, let each party compute a partial aggregation,
  // 2. each party send the partial result to active party for fully aggregation,
  // 3. the active party add the bias term, and broadcast the encrypted outputs
  log_info("[comp_other_layer_agg_output] compute the encrypted aggregation for the other hidden layer");
  int prev_neuron_size = (int) prev_layer_outputs_shares.size();
  int prev_batch_size = (int) prev_layer_outputs_shares[0].size();
  if ((prev_neuron_size != m_num_inputs_per_neuron) ||
      (prev_batch_size != cur_batch_size) || (output_size != m_num_neurons)) {
    log_error("[comp_other_layer_agg_output] size does not match");
    exit(EXIT_FAILURE);
  }

  // local aggregation
  auto** encoded_prev_outputs_shares = new EncodedNumber*[prev_batch_size];
  for (int i = 0; i < prev_batch_size; i++) {
    encoded_prev_outputs_shares[i] = new EncodedNumber[prev_neuron_size];
  }
  encode_samples(party, prev_layer_outputs_shares, encoded_prev_outputs_shares);

  int precision = std::abs(encoded_prev_outputs_shares[0][0].getter_exponent())
      + std::abs(m_neurons[0].m_weights[0].getter_exponent());

  // transform matrix helper
  auto **res_helper = new EncodedNumber*[m_num_neurons];
  for (int i = 0; i < m_num_neurons; i++) {
    res_helper[i] = new EncodedNumber[cur_batch_size];
  }

  for (int i = 0; i < m_num_neurons; i++) {
    m_neurons[i].compute_batch_phe_aggregation_with_shares(
        party, cur_batch_size, prev_neuron_size,
        encoded_prev_outputs_shares, precision, res_helper[i]);
  }

  // convert res_helper into res
  for (int i = 0; i < cur_batch_size; i++) {
    for (int j = 0; j < m_num_neurons; j++) {
      res[i][j] = res_helper[j][i];
    }
  }

  // free memory
  for (int i = 0; i < m_num_neurons; i++) {
    delete [] res_helper[i];
  }
  delete [] res_helper;

  for (int i = 0; i < prev_batch_size; i++) {
    delete [] encoded_prev_outputs_shares[i];
  }
  delete [] encoded_prev_outputs_shares;
}

void Layer::update_encrypted_weights(const Party &party,
                                     const std::vector<double> &deriv_error,
                                     double m_learning_rate,
                                     std::vector<double> *deltas) {

}