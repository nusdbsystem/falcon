//
// Created by root on 11/11/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_MODEL_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_MODEL_H_

#include "falcon/distributed/worker.h"
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/common.h>
#include <falcon/party/party.h>

#include <future>
#include <thread>

class LinearRegressionModel : public LinearModel {
public:
  LinearRegressionModel();
  explicit LinearRegressionModel(int m_weight_size);
  ~LinearRegressionModel();

  /**
   * copy constructor
   * @param linear_reg_model
   */
  LinearRegressionModel(const LinearRegressionModel &linear_reg_model);

  /**
   * assignment constructor
   * @param linear_reg_model
   * @return
   */
  LinearRegressionModel &
  operator=(const LinearRegressionModel &linear_reg_model);

  /**
   * given the logistic regression model, predict on samples
   * @param party
   * @param predicted_samples
   * @param predicted_sample_size
   * @param predicted_labels
   * @return predicted labels (encrypted)
   */
  void predict(const Party &party,
               const std::vector<std::vector<double>> &predicted_samples,
               EncodedNumber *predicted_labels) const;

  /**
   * forward calculate of the network，output predicted_labels
   *
   * @param party: initialized party object
   * @param data_indexes: used training data indexes
   * @param batch_logistic_shares: secret shares of batch losses
   * @param encrypted_weights_precision: precision
   * @param plaintext_samples_precision: precision
   */
  void forward_computation(const Party &party, int cur_batch_size,
                           EncodedNumber **encoded_batch_samples,
                           int &encrypted_batch_aggregation_precision,
                           EncodedNumber *predicted_labels) const;
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_MODEL_H_
