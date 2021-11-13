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


#endif //FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BUILDER_HELPER_H_
