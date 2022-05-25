//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_H_

#include <falcon/common.h>
#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/nn/neuron.h>
#include <falcon/algorithm/vertical/nn/layer.h>

// the mlp model
class MLP {
 public:
  // the number of inputs (input layer size)
  int m_num_inputs;
  // the number of outputs (output layer size)
  int m_num_outputs;
  // the number of hidden_layers
  int m_num_hidden_layers;
  // the number of neurons in each layer
  std::vector<int> m_num_layers_neurons;
  // the vector of layers
  std::vector<Layer> m_layers;

 public:
  /**
   * default constructor
   */
  MLP();

  /**
   * constructor
   * @param with_bias: whether has the bias term
   * @param num_layers_neurons: the vector of layers neuron numbers
   * @param layers_activation_funcs: the vector of layers activation functions
   */
  MLP(bool with_bias, const std::vector<int>& num_layers_neurons, const std::vector<std::string>& layers_activation_funcs);

  /**
   * default destructor
   */
  ~MLP();
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_H_
