//
// Created by root on 5/21/22.
//

#include <falcon/algorithm/vertical/nn/neuron.h>

Neuron::Neuron() {
  m_num_inputs = 0;
  m_fit_bias = true;
}

Neuron::Neuron(int num_inputs, bool with_bias) {
  m_num_inputs = num_inputs;
  m_fit_bias = with_bias;
  m_bias = new EncodedNumber[1];
  m_weights = new EncodedNumber[m_num_inputs];
}

Neuron::~Neuron() {
  m_num_inputs = 0;
  m_fit_bias = true;
  delete [] m_bias;
  delete [] m_weights;
}