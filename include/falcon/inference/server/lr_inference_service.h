//
// Created by wuyuncheng on 4/6/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_SERVER_INFERENCE_SERVER_LR_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_SERVER_INFERENCE_SERVER_LR_H_

#include <string>
#include <falcon/party/party.h>

/**
 * run grpc server to listen client requests
 *
 * @param endpoint: listened endpoint
 * @param saved_model_file: saved model
 * @param party: initialized party
 */
void run_active_server_lr(const std::string& endpoint,
    const std::string& saved_model_file,
    const Party& party);

/**
 * run passive server to listen active party's requests
 *
 * @param saved_model_file: saved model
 * @param party: initialized party
 */
void run_passive_server_lr(const std::string& saved_model_file,
    const Party& party);

#endif //FALCON_INCLUDE_FALCON_INFERENCE_SERVER_INFERENCE_SERVER_LR_H_
