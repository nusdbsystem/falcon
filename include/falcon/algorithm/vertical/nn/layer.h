//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_

#include <falcon/common.h>
#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/nn/neuron.h>

// layer of an MLP model
class Layer {
 public:
  // the number of neurons of this layer
  int m_num_neurons;
  // the number of inputs for each neuron
  int m_num_inputs_per_neuron;
  // the neurons vector
  std::vector<Neuron> m_neurons;
  // the activation function string: sigmoid, linear, etc.
  std::string m_activation_func_str;

 public:
  /**
   * default constructor
   */
  Layer();

  /**
   * constructor
   * @param num_neurons: the number of neurons in this layer
   * @param num_inputs_per_neuron: the number of inputs for each neuron
   * @param with_bias: whether has bias term
   * @param activation_func_str: the activation function
   */
  Layer(int num_neurons, int num_inputs_per_neuron,
        bool with_bias, const std::string& activation_func_str);

  /**
   * copy constructor
   * @param layer
   */
  Layer(const Layer &layer);

  /**
   * assignment constructor
   * @param layer
   * @return
   */
  Layer &operator=(const Layer &layer);

  /**
   * default destructor
   */
  ~Layer();
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_
