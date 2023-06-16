//
// Created by root on 11/13/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_

#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/common.h>
#include <falcon/distributed/worker.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/party/party.h>
#include <falcon/utils/io_util.h>

#include <future>
#include <string>
#include <thread>
#include <vector>

/**
 * select batch indexes for each iteration
 *
 * @param party: initialized party object
 * @param batch_size: the batch size in the builder
 * @param data_indexes: the original training data indexes
 * @return
 */
std::vector<int> sync_batch_idx(const Party &party, int batch_size,
                                std::vector<int> data_indexes);

/**
 * this function pre-compute all the needed batch indexes for each iteration
 *
 * @param batch_size: the batch size in the builder
 * @param max_iter: the maximum number iterations
 * @param data_indexes: the original training data indexes
 * @return
 */
std::vector<std::vector<int>>
precompute_iter_batch_idx(int batch_size, int max_iter,
                          std::vector<int> data_indexes);

/**
 * initialize encrypted weights for linear models
 *
 * @param party: initialized party object
 * @param party_weight_sizes: party's weight size vector
 * @param encrypted_vector: the encrypted values returned
 * @param precision: precision for big integer representation EncodedNumber
 */
void init_encrypted_model_weights(const Party &party,
                                  std::vector<int> party_weight_sizes,
                                  EncodedNumber *encrypted_vector,
                                  int precision = PHE_FIXED_POINT_PRECISION);

/**
 * split the training data and testing data according to the fit_bias flag
 *
 * @param party: initialized party object
 * @param fit_bias: whether enable bias term
 * @param training_data: returned training data
 * @param testing_data: returned testing data
 * @param training_labels: returned training labels
 * @param testing_labels: returned testing labels
 * @param split_percentage: the split percentage of the data
 */
void split_dataset(Party *party, bool fit_bias,
                   std::vector<std::vector<double>> &training_data,
                   std::vector<std::vector<double>> &testing_data,
                   std::vector<double> &training_labels,
                   std::vector<double> &testing_labels,
                   double split_percentage = SPLIT_TRAIN_TEST_RATIO);

/**
 * compute the encrypted residual, given the encrypted predicted labels,
 * and the ground truth plaintext labels, the active party broadcast the
 * result to all parties after computation
 *
 * @param party: initialized party object
 * @param batch_indexes: the batch indexes
 * @param batch_true_labels: the batch encrypted training data labels
 * @param precision: the ciphertext precision
 * @param predicted_labels: the encrypted predicted labels
 * @param encrypted_batch_losses: return, computed encrypted batch loss
 */
void compute_encrypted_residual(const Party &party,
                                const std::vector<int> &batch_indexes,
                                EncodedNumber *batch_true_labels, int precision,
                                EncodedNumber *predicted_labels,
                                EncodedNumber *encrypted_batch_losses);

/**
 * encode data samples,
 *
 * @param party: initialized party object
 * @param used_samples: used data_samples
 * @param encoded_samples: result value
 * @param precision: the precision of the encoded samples
 */
void encode_samples(const Party &party,
                    const std::vector<std::vector<double>> &used_samples,
                    EncodedNumber **encoded_samples,
                    int precision = PHE_FIXED_POINT_PRECISION);

/**
 * get the two-dimensional encrypted true labels
 *
 * @param party: initialized party object
 * @param output_layer_size: the number of neurons of the last layer (decide
 * regression or classification)
 * @param plain_batch_labels: the plaintext batch labels
 * @param batch_true_labels: the encrypted batch true labels
 * @param precision: the precision of the returned batch_true_labels
 */
void get_encrypted_2d_true_labels(const Party &party, int output_layer_size,
                                  const std::vector<double> &plain_batch_labels,
                                  EncodedNumber **batch_true_labels,
                                  int precision);

void display_encrypted_matrix(const Party &party, int row_size, int col_size,
                              EncodedNumber **mat);

void display_encrypted_vector(const Party &party, int size, EncodedNumber *vec);

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_
