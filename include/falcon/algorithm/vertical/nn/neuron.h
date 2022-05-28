//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_NEURON_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_NEURON_H_

#include <falcon/common.h>
#include <falcon/party/party.h>

// neuron of a layer
class Neuron {
 public:
  // number of inputs to this neuron
  int m_num_inputs;
  // whether this neuron has bias (default true)
  bool m_fit_bias;
  // encrypted bias (size = 1)
  EncodedNumber* m_bias{};
  // weights vector, encrypted values during training, size = m_num_inputs
  EncodedNumber* m_weights{};

 public:
  /**
   * default constructor
   */
  Neuron();

  /**
   * constructor
   * @param num_inputs: the number of inputs of this neuron
   * @param fit_bias: whether this neuron has bias
   */
  Neuron(int num_inputs, bool fit_bias);

  /**
   * copy constructor
   * @param neuron
   */
  Neuron(const Neuron &neuron);

  /**
   * assignment constructor
   * @param neuron
   * @return
   */
  Neuron &operator=(const Neuron &neuron);

  /**
   * default destructor
   */
  ~Neuron();

  /**
   * initialize encrypted local weights
   *
   * @param party: initialized party object
   * @param precision: precision for big integer representation EncodedNumber
   */
  void init_encrypted_weights(const Party& party, int precision);

  /**
   * update the layer encrypted weights
   *
   * @param party: initialized party object
   * @param deriv_error: the derived error from the previous layer
   * @param m_learning_rate: the learning rate
   * @param deltas: the deltas returned
   */
  void update_encrypted_weights(const Party& party,
                                const std::vector<double>& deriv_error,
                                double m_learning_rate,
                                std::vector<double> *deltas);
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_NEURON_H_
