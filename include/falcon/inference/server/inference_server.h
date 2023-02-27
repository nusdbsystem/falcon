//
// Created by wuyuncheng on 9/3/21.
//

#ifndef FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
#define FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_

#include <falcon/party/party.h>
#include <string>

/**
 * launch a server, processing logic is defined in processing_func
 *
 * @param endpoint: listened endpoint
 * @param saved_model_file: saved model
 * @param party: initialized party
 * @param algorithm_name: the model to be served
 * @param is_distributed: whether it is a distributed training or inference
 * @param ps_network_config_pb_str: if is_distributed = 1, given the ps and
 * worker network config
 */
void run_inference_server(const std::string &endpoint,
                          const std::string &saved_model_file,
                          const Party &party,
                          falcon::AlgorithmName algorithm_name,
                          int is_distributed,
                          const std::string &ps_network_config_pb_str);

/**
 * run grpc server to listen client requests
 *
 * @param endpoint: listened endpoint
 * @param saved_model_file: saved model
 * @param party: initialized party
 * @param algorithm_name: the model to be served
 */
void run_active_server(const Party &party, const std::string &saved_model_file,
                       const std::vector<int> &batch_indexes,
                       falcon::AlgorithmName algorithm_name,
                       EncodedNumber *decrypted_labels);

/**
 * run passive server to listen active party's requests
 *
 * @param saved_model_file: saved model
 * @param party: initialized party
 * @param algorithm_name: the model to be served
 */
void run_passive_server(const std::string &saved_model_file, const Party &party,
                        falcon::AlgorithmName algorithm_name);

#endif // FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
