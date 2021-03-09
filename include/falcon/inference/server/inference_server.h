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
 */
void RunServer(const std::string& endpoint,
               const std::string& saved_model_file,
               const Party& party);

/**
 * run passive server to listen active party's requests
 *
 * @param saved_model_file: saved model
 * @param party: initialized party
 */
void RunPassiveServer(const std::string& saved_model_file, Party party);

#endif //FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
