//
// Created by root on 11/13/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BASE_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BASE_H_

#include <falcon/common.h>
#include <falcon/party/party.h>
#include "falcon/distributed/worker.h"

#include <thread>
#include <future>

class LinearModel {
 public:
  // number of weights in the model
  int weight_size;
  // model weights vector, encrypted values during training, size equals to weight_size
  EncodedNumber *local_weights{};

 public:
  LinearModel();
  explicit LinearModel(int m_weight_size);
  ~LinearModel();

  /**
   * copy constructor
   * @param linear_model
   */
  LinearModel(const LinearModel &linear_model);

  /**
   * assignment constructor
   * @param log_reg_model
   * @return
   */
  LinearModel &operator=(const LinearModel &linear_model);

  /**
 * compute phe aggregation for a batch of samples
 *
 * @param party: initialized party object
 * @param batch_indexes: selected batch indexes
 * @param dataset_type: denote the dataset type
 * @param precision: the fixed point precision of encoded plaintext samples
 * @param batch_aggregation: returned phe aggregation for the batch
 */
  void compute_batch_phe_aggregation(
      const Party &party,
      int cur_batch_size,
      EncodedNumber** encoded_batch_samples,
      int precision,
      EncodedNumber *encrypted_batch_aggregation) const;

  /**
   * encode data samples,
   *
   * @param party: initialized party object
   * @param used_samples: used data_samples
   * @param encoded_samples: result value
   * @param precision: the precision of the encoded samples
   */
  void encode_samples(
      const Party &party,
      const std::vector<std::vector<double>>& used_samples,
      EncodedNumber** encoded_samples,
      int precision = PHE_FIXED_POINT_PRECISION) const;
};

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BASE_H_
