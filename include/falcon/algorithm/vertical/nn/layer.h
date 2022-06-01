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

  /**
   * initialize encrypted local weights
   *
   * @param party: initialized party object
   * @param precision: precision for big integer representation EncodedNumber
   */
  void init_encrypted_weights(const Party& party, int precision);

  /**
   * compute the aggregation of the first hidden layer
   *
   * @param party: initialized party object
   * @param cur_batch_size: the number of samples of the current batch
   * @param local_weight_sizes: the local weight size of each party
   * @param encoded_batch_samples: the encoded batch samples
   * @param output_size: the size of the resulted encrypted aggregation res
   * @param res: encrypted aggregation result
   */
  void comp_1st_layer_agg_output(const Party& party,
                                 int cur_batch_size,
                                 const std::vector<int>& local_weight_sizes,
                                 EncodedNumber **encoded_batch_samples,
                                 int output_size,
                                 EncodedNumber **res) const;

  /**
   * compute the aggregation of the other hidden layers
   *
   * @param party: initialized party object
   * @param cur_batch_size: the number of samples of the current batch
   * @param prev_layer_outputs_shares: the output shares of the previous layer
   * @param output_size: the size of the resulted encrypted aggregation res
   * @param res: encrypted aggregation result
   */
  void comp_other_layer_agg_output(const Party& party,
                                   int cur_batch_size,
                                   const std::vector<std::vector<double>>& prev_layer_outputs_shares,
                                   int output_size,
                                   EncodedNumber **res) const;

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

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_
