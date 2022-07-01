//
// Created by wuyuncheng on 7/7/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_MODEL_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_MODEL_H_

#include <falcon/common.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/party/party.h>
#include "falcon/distributed/worker.h"

#include <thread>
#include <future>

class LogisticRegressionModel : public LinearModel {
 public:
  LogisticRegressionModel();
  explicit LogisticRegressionModel(int m_weight_size);
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
  void predict(const Party &party,
      const std::vector<std::vector<double> >& predicted_samples,
      EncodedNumber *predicted_labels) const;

  /**
  * given the logistic regression model, predict on samples with probabilities
  * @param party
  * @param predicted_samples
  * @param predicted_sample_size
  * @param predicted_labels
  * @return predicted labels (encrypted)
  */
  void predict_proba(const Party &party, std::vector<std::vector<double> > predicted_samples,
               EncodedNumber **predicted_labels) const;

  /**
   * forward calculate of the network，output predicted_labels
   *
   * @param party: initialized party object
   * @param data_indexes: used training data indexes
   * @param batch_logistic_shares: secret shares of batch losses
   * @param encrypted_weights_precision: precision
   * @param plaintext_samples_precision: precision
   */
  void forward_computation(
      const Party& party,
      int cur_batch_size,
      EncodedNumber** encoded_batch_samples,
      int& encrypted_batch_aggregation_precision,
      EncodedNumber *predicted_labels) const;
};


/**
  * retrieve result by collaborative decrypting and compute labels and probabilities
  *
  * @param party: initialized party object
  * @param sample_size: size of samples
  * @param predicted_labels: predicted_labels
  * @param labels: return value
  * @param probabilities: return value
  */
void retrieve_prediction_result(
    int sample_size,
    EncodedNumber *decrypted_labels,
    std::vector<double>* labels,
    std::vector< std::vector<double> >* probabilities);

/**
 * spdz computation with thread,
 * the spdz_logistic_function_computation will do the 1/(1+e^(wx)) operation
 *
 * @param party_num: number of parties
 * @param party_id: current party id
 * @param mpc_port_bases: port bases of the spdz parties
 * @param mpc_player_path: player data path of the spdz parties
 * @param party_host_names: spdz parties host names (ips)
 * @param batch_aggregation_shares: the batch shares
 * @param cur_batch_size: size of current batch
 * @param batch_loss_shares: promise structure of the loss shares
 */
void spdz_logistic_function_computation(int party_num,
    int party_id,
    std::vector<int> mpc_port_bases,
    const std::string& mpc_player_path,
    std::vector<std::string> party_host_names,
    std::vector<double> batch_aggregation_shares,
    int cur_batch_size,
    falcon::SpdzLogRegCompType comp_type,
    std::promise<std::vector<double>> *batch_loss_shares);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LOGISTIC_REGRESSION_MODEL_H_
