//
// Created by naili on 21/8/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_H_

#include <string>
#include <vector>
#include <falcon/party/party.h>


class WeightedPearson {

 public:
  WeightedPearson() = default;
  ~WeightedPearson() = default;

  /**
   * This function return importance of features with encrypted predictions,
   * and encrypted sample_weights.
   *
   * @param party: the participating party
   * @param class_id: the class id to be explained
   * @param train_data: the plaintext train data
   * @param predictions: the encrypted model predictions
   * @param sss_sample_weights: the sss sample weights
   * @param ps_network_str: the parameters of ps network string,
   * @param is_distributed: whether use distributed interpretable model training
   * @param distributed_role: if is_distributed = 1, meaningful; if 0, ps, else: worker
   * @param worker_id: if is_distributed = 1 and distributed_role = 1
   * @return
   */
  std::vector<double> get_feature_importance(
      Party party,
      int class_id,
      const std::vector<std::vector<double>>& train_data,
      EncodedNumber* predictions,
      const std::vector<double> &sss_sample_weights,
      const std::string& ps_network_str = std::string(),
      int is_distributed = 0,
      int distributed_role = 0,
      int worker_id = 0);

  /**
   * get the correlation
   *
   * @param party: the participating party
   * @param class_id: the class id to be explained
   * @param train_data: the plaintext train data
   * @param predictions: the encrypted model predictions
   * @param sss_sample_weights: the sss sample weights
   * @return correlation
   */
  std::vector<double> get_correlation(
      Party party,
      int class_id,
      const vector<std::vector<double>> &train_data,
      EncodedNumber *predictions,
      const vector<double> &sss_sample_weights);

};


void spdz_lime_divide(int party_num,
                      int party_id,
                      std::vector<int> mpc_port_bases,
                      std::vector<std::string> party_host_names,
                      int public_value_size,
                      const std::vector<int>& public_values,
                      int private_value_size,
                      const std::vector<double>& private_values,
                      falcon::SpdzLimeCompType lime_comp_type,
                      std::promise<std::vector<double>> *res);

void spdz_lime_WPCC(int party_num,
                    int party_id,
                    std::vector<int> mpc_port_bases,
                    std::vector<std::string> party_host_names,
                    int public_value_size,
                    const std::vector<int>& public_values,
                    int private_value_size,
                    const std::vector<double>& private_values,
                    falcon::SpdzLimeCompType lime_comp_type,
                    std::promise<std::vector<double>> *res);


#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_WEIGHTED_PEARSON_H_
