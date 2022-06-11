//
// Created by root on 5/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_H_

#include <falcon/common.h>
#include <falcon/party/party.h>
#include <falcon/algorithm/vertical/nn/layer.h>
#include <thread>
#include <future>
#include <numeric>
#include <iostream>
#include <chrono>

typedef std::vector<std::vector<std::vector<double>>> TripleDVec;

// the mlp model
class MlpModel {
 public:
  // the number of inputs (input layer size)
  int m_num_inputs;
  // the number of outputs (output layer size)
  int m_num_outputs;
  // the number of hidden_layers
  int m_num_hidden_layers;
  // the number of neurons in each layer
  std::vector<int> m_layers_num_outputs;
  // the vector of layers
  std::vector<Layer> m_layers;

 public:
  /**
   * default constructor
   */
  MlpModel();

  /**
   * constructor
   * @param with_bias: whether has the bias term
   * @param layers_num_outputs: the vector of layers neuron numbers
   * @param layers_activation_funcs: the vector of layers activation functions
   */
  MlpModel(bool with_bias, const std::vector<int>& layers_num_outputs, const std::vector<std::string>& layers_activation_funcs);

  /**
   * copy constructor
   * @param mlp_model
   */
  MlpModel(const MlpModel &mlp_model);

  /**
   * assignment constructor
   * @param mlp_model
   * @return
   */
  MlpModel &operator=(const MlpModel &mlp_model);

  /**
   * default destructor
   */
  ~MlpModel();

  /**
   * init the encrypted weights of the model
   *
   * @param party: initialized party object
   * @param precision: precision for big integer representation EncodedNumber
   */
  void init_encrypted_weights(const Party& party, int precision = PHE_FIXED_POINT_PRECISION);

  /**
   * give the mlp model, predict the labels on samples
   *
   * @param party: initialized party object
   * @param predicted_samples: the samples to be predicted
   * @param predicted_labels: the predicted labels of the encoded samples
   */
  void predict(const Party& party,
               const std::vector<std::vector<double>>& predicted_samples,
               EncodedNumber *predicted_labels) const;

  /**
   * give the mlp model, predict the probabilities on samples
   *
   * @param party: initialized party object
   * @param predicted_samples: the samples to be predicted
   * @param predicted_labels: the predicted probabilities of the encoded samples
   */
  void predict_proba(const Party& party,
                     const std::vector<std::vector<double>>& predicted_samples,
                     EncodedNumber **predicted_labels) const;

  /**
   * forward calculation of the network, output predicted labels
   * which consists of the prediction probability of each sample
   *
   * @param party: initialized party object
   * @param cur_batch_size: the number of samples of the current batch
   * @param local_weight_sizes: the local weights of parties
   * @param encoded_batch_samples: the encoded batch samples
   * @param predicted_labels: the predicted probabilities of the encoded samples
   * @param layer_activation_shares: return the activation shares of each layer
   * @param layer_deriv_activation_shares: return the derivative activation shares of each layer
   */
  void forward_computation(
      const Party& party,
      int cur_batch_size,
      const std::vector<int>& local_weight_sizes,
      EncodedNumber** encoded_batch_samples,
      EncodedNumber** predicted_labels,
      TripleDVec& layer_activation_shares,
      TripleDVec& layer_deriv_activation_shares) const;

  /**
   * parse the resulted shares received from spdz program in forward_computation
   *
   * @param res: the received values from spdz
   * @param dim1_size: the size of first dimension
   * @param dim2_size: the size of second dimension
   * @param act_outputs_shares: the activation secret shares
   * @param deriv_act_outputs_shares: the derivative activation secret shares
   */
  static void parse_spdz_act_comp_res(
      const std::vector<double>& res,
      int dim1_size,
      int dim2_size,
      std::vector<std::vector<double>>& act_outputs_shares,
      std::vector<std::vector<double>>& deriv_act_outputs_shares) ;

  /**
   * forward calculation of the network, output predicted labels
   * which consists of the prediction probability of each sample
   *
   * @param party: initialized party object
   * @param cur_batch_size: the number of samples of the current batch
   * @param local_weight_sizes: the local weights of parties
   * @param encoded_batch_samples: the encoded batch samples
   * @param predicted_labels: the predicted probabilities of the encoded samples
   */
  void forward_computation_fast(
      const Party& party,
      int cur_batch_size,
      const std::vector<int>& local_weight_sizes,
      EncodedNumber** encoded_batch_samples,
      EncodedNumber** predicted_labels) const;
};

/**
    * compute spdz function for mlp builder
    *
    * @param party_num: the number of parties
    * @param party_id: the party's id
    * @param mpc_tree_port_base: the port base to connect to mpc program
    * @param party_host_names: the ip addresses of parties
    * @param public_value_size: the number of public values
    * @param public_values: the public values to be sent
    * @param private_value_size: the number of private values
    * @param private_values: the private values to be sent
    * @param mlp_comp_type: the computation type
    * @param res: the result returned from mpc program
 */
void spdz_mlp_computation(int party_num,
                          int party_id,
                          std::vector<int> mpc_port_bases,
                          std::vector<std::string> party_host_names,
                          int public_value_size,
                          const std::vector<int>& public_values,
                          int private_value_size,
                          const std::vector<double>& private_values,
                          falcon::SpdzMlpCompType mlp_comp_type,
                          std::promise<std::vector<double>> *res);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_NN_MLP_H_
