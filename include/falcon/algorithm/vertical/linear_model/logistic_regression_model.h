//
// Created by wuyuncheng on 7/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_MODEL_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_MODEL_H_

#include <falcon/common.h>
#include <falcon/party/party.h>

#include <thread>
#include <future>

class LogisticRegressionModel {
 public:
  // number of weights in the model
  int weight_size;
  // model weights vector, encrypted values during training, size equals to weight_size
  EncodedNumber* local_weights{};

 public:
  LogisticRegressionModel();
  LogisticRegressionModel(int m_weight_size);
  ~LogisticRegressionModel();

  /**
   * copy constructor
   * @param log_reg_model
   */
  LogisticRegressionModel(const LogisticRegressionModel &log_reg_model);

  /**
   * assignment constructor
   * @param log_reg_model
   * @return
   */
  LogisticRegressionModel &operator=(const LogisticRegressionModel &log_reg_model);

  /**
 * given the logistic regression model, predict on samples
 * @param party
 * @param predicted_samples
 * @param predicted_sample_size
 * @param predicted_labels
 * @return predicted labels (encrypted)
 */
  void predict(Party& party,
      std::vector< std::vector<double> > predicted_samples,
      int predicted_sample_size,
      EncodedNumber* predicted_labels);
};

/**
 * spdz computation with thread
 *
 * @param party_num: number of parties
 * @param party_id: current party id
 * @param mpc_port_base: port base of the spdz parties
 * @param mpc_player_path: player data path of the spdz parties
 * @param party_host_names: spdz parties host names (ips)
 * @param batch_aggregation_shares: the batch shares
 * @param cur_batch_size: size of current batch
 * @param batch_loss_shares: promise structure of the loss shares
 */
void spdz_logistic_function_computation(int party_num,
    int party_id,
    int mpc_port_base,
    std::string mpc_player_path,
    std::vector<std::string> party_host_names,
    std::vector<double> batch_aggregation_shares,
    int cur_batch_size,
    std::promise<std::vector<double>> *batch_loss_shares);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_MODEL_H_
