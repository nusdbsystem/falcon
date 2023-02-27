//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_

#include <falcon/common.h>
#include <falcon/party/party.h>

// layer of an MLP model
class Layer {
public:
  // the number of inputs for each neuron in this layer
  // in fact, it is the number of neurons (number of outputs) of the previous
  // layer
  int m_num_inputs;
  // the number of outputs for this layer
  int m_num_outputs;
  // whether the neurons of this layer have bias (default true)
  bool m_fit_bias;
  // the activation function string: sigmoid, linear, etc.
  // the activation function of the output layer needs to match
  // the loss function defined in the mlp builder
  std::string m_activation_func_str;
  // the weight matrix, encrypted values during training, dimension =
  // (m_num_inputs, m_num_outputs)
  EncodedNumber **m_weight_mat{};
  // the bias vector, encrypted values during training, dimension =
  // m_num_outputs
  EncodedNumber *m_bias{};

public:
  /**
   * default constructor
   */
  Layer();

  /**
   * constructor
   * @param num_inputs: the number of inputs for each neuron in this layer
   * @param num_outputs: the number of outputs for this layer
   * @param with_bias: whether has bias term
   * @param activation_func_str: the activation function
   */
  Layer(int num_inputs, int num_outputs, bool with_bias,
        const std::string &activation_func_str);

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
  void init_encrypted_weights(const Party &party, int precision);

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
  void comp_1st_layer_agg_output(const Party &party, int cur_batch_size,
                                 const std::vector<int> &local_weight_sizes,
                                 EncodedNumber **encoded_batch_samples,
                                 int output_size, EncodedNumber **res) const;

  /**
   * compute the aggregation of the other hidden layers
   *
   * @param party: initialized party object
   * @param cur_batch_size: the number of samples of the current batch
   * @param prev_layer_outputs_shares: the output shares of the previous layer
   * @param output_size: the size of the resulted encrypted aggregation res
   * @param res: encrypted aggregation result
   */
  void comp_other_layer_agg_output(
      const Party &party, int cur_batch_size,
      const std::vector<std::vector<double>> &prev_layer_outputs_shares,
      int output_size, EncodedNumber **res) const;
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_MLP_LAYER_H_
