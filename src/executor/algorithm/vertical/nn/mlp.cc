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

MlpModel::~MlpModel() {
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_num_hidden_layers = 0;
  m_num_layers_neurons.clear();
  m_layers.clear();
}