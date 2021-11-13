//
// Created by root on 11/13/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_

#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/algorithm/model_builder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/party/party.h>
#include <falcon/common.h>
#include <falcon/distributed/worker.h>
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
std::vector<int> select_batch_idx(const Party& party,
                                  int batch_size,
                                  std::vector<int> data_indexes);

/**
 * initialize encrypted random numbers
 *
 * @param party: initialized party object
 * @param vector_size: the number of random values
 * @param encrypted_vector: the encrypted values returned
 * @param precision: precision for big integer representation EncodedNumber
 */
void init_encrypted_random_numbers(const Party& party,
                            int vector_size,
                            EncodedNumber* encrypted_vector,
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
void split_dataset(Party* party,
                   bool fit_bias,
                   std::vector<std::vector<double> >& training_data,
                   std::vector<std::vector<double> >& testing_data,
                   std::vector<double>& training_labels,
                   std::vector<double>& testing_labels,
                   double split_percentage = SPLIT_TRAIN_TEST_RATIO);

#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_
