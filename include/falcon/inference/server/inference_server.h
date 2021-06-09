//
// Created by wuyuncheng on 9/3/21.
//

#ifndef FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
#define FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_

#include <string>
#include <falcon/party/party.h>

/**
 * run grpc server to listen client requests
 *
 * @param endpoint: listened endpoint
 * @param saved_model_file: saved model
 * @param party: initialized party
 * @param algorithm_name: the model to be served
 */
void run_active_server(const std::string& endpoint,
               const std::string& saved_model_file,
               const Party& party,
               falcon::AlgorithmName algorithm_name);

/**
 * run passive server to listen active party's requests
 *
 * @param saved_model_file: saved model
 * @param party: initialized party
 * @param algorithm_name: the model to be served
 */
void run_passive_server(const std::string& saved_model_file,
                const Party& party,
                falcon::AlgorithmName algorithm_name);

#endif //FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
