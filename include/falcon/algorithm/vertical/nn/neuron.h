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
   * @param limit: assist to generate random weights
   * @param precision: precision for big integer representation EncodedNumber
   */
  void init_encrypted_weights(const Party& party, double limit, int precision);

  /**
    * compute phe aggregation for a batch of samples
    *
    * @param party: initialized party object
    * @param cur_batch_size: the batch size of the current iteration
    * @param local_weight_sizes: the number of local weights of parties
    * @param encoded_batch_samples: the encoded batch samples
    * @param precision: the precision of the ciphertexts
    * @param batch_aggregation: returned phe aggregation for the batch
    */
  void compute_batch_phe_aggregation(
      const Party &party,
      int cur_batch_size,
      const std::vector<int>& local_weight_sizes,
      EncodedNumber** encoded_batch_samples,
      int precision,
      EncodedNumber *encrypted_batch_aggregation) const;

  /**
   * compute the phe aggregation for a batch of secret shares
   *
   * @param party: initialized party object
   * @param cur_batch_size: the batch size of the current iteration
   * @param prev_neuron_size: the number of neurons in previous layer
   * @param encoded_batch_shares: the encoded secret shares
   * @param precision: the precision of the ciphertexts
   * @param batch_aggregation: returned phe aggregation for the batch
   */
  void compute_batch_phe_aggregation_with_shares(
      const Party& party,
      int cur_batch_size,
      int prev_neuron_size,
      EncodedNumber** encoded_batch_shares,
      int precision,
      EncodedNumber* encrypted_batch_aggregation) const;

  void sum_batch_local_aggregations(
      const Party& party,
      EncodedNumber* batch_local_aggregation,
      int precision,
      int cur_batch_size,
      EncodedNumber* encrypted_batch_aggregation) const;

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
