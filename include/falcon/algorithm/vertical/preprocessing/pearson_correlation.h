//
// Created by naili on 22/6/22.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_PEARSON_CORRELATION_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_PEARSON_CORRELATION_H_
#include <string>
#include <vector>

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/party/party.h>

class PearsonCorrelation {

public:
  PearsonCorrelation() = default;
  ~PearsonCorrelation() = default;

  /**
   * calculate score for each feature using weighted pearson correlation
   * algorithm
   * @param encrypted_samples samples of two-D array
   * @param encrypted_labels labels of activate party
   * @param sample_size size of the vector
   * @param encrypted_weight weights of features, (optional)
   * @return
   */
  EncodedNumber calculate_score(const Party &party,
                                std::vector<double> plain_samples,
                                EncodedNumber *encrypted_labels,
                                int sample_size,
                                EncodedNumber *encrypted_weight = NULL);

  /**
   * calculate weighted mean between cipher and plain
   * @param encrypted_weight
   * @param encrypted_samples
   * @param sample_size
   * @return
   */
  EncodedNumber calculate_weighted_mean(const Party &party,
                                        EncodedNumber *encrypted_weight,
                                        std::vector<double> plain_samples,
                                        int sample_size);

  /**
   * calculate weighted mean between ciphers
   * @param encrypted_weight
   * @param encrypted_samples
   * @param sample_size
   * @return
   */
  EncodedNumber calculate_weighted_mean(const Party &party,
                                        EncodedNumber *encrypted_weight,
                                        EncodedNumber *encrypted_samples,
                                        int sample_size);

  /**
   * calculate weighted variance
   * @param encrypted_weight
   * @param encrypted_samples
   * @param mean
   * @param sample_size
   * @return
   */
  EncodedNumber calculate_weighted_variance(const Party &party,
                                            EncodedNumber *encrypted_weight,
                                            EncodedNumber *encrypted_samples,
                                            EncodedNumber mean,
                                            int sample_size);

  /**
   * calculate weighted covariance
   * @param encrypted_weight
   * @param encrypted_samples
   * @param mean_sample
   * @param encrypted_labels
   * @param mean_label
   * @param sample_size
   * @return
   */
  EncodedNumber calculate_weight_covariance(const Party &party,
                                            EncodedNumber *encrypted_weight,
                                            std::vector<double> plain_samples,
                                            EncodedNumber mean_sample,
                                            EncodedNumber *encrypted_labels,
                                            EncodedNumber mean_label,
                                            int sample_size);
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_PREPROCESSING_PEARSON_CORRELATION_H_
