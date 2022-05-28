//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/vertical/nn/mlp.h>

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

void MlpModel::predict(const Party &party,
                       const std::vector<std::vector<double>> &predicted_samples,
                       EncodedNumber *predicted_labels) const {

}

void MlpModel::predict_proba(const Party &party,
                             const std::vector<std::vector<double>> &predicted_samples,
                             EncodedNumber **predicted_labels) const {

}

void MlpModel::forward_computation(const Party &party,
                                   int cur_batch_size,
                                   EncodedNumber **encoded_batch_samples,
                                   EncodedNumber **predicted_labels) const {

}

void spdz_mlp_computation(int party_num,
                          int party_id,
                          std::vector<int> mpc_port_bases,
                          const std::string& mpc_player_path,
                          std::vector<std::string> party_host_names,
                          int public_value_size,
                          const std::vector<int>& public_values,
                          int private_value_size,
                          const std::vector<double>& private_values,
                          falcon::SpdzMlpCompType mlp_comp_type,
                          std::promise<std::vector<double>> *res) {

}