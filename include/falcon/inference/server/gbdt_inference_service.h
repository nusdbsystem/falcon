//
// Created by wuyuncheng on 11/8/21.
//

#ifndef FALCON_INCLUDE_FALCON_INFERENCE_SERVER_GBDT_INFERENCE_SERVICE_H_
#define FALCON_INCLUDE_FALCON_INFERENCE_SERVER_GBDT_INFERENCE_SERVICE_H_

#include <falcon/algorithm/vertical/tree/gbdt_model.h>
#include <falcon/party/party.h>
#include <string>

/**
 * run grpc server to listen client requests
 *
 * @param endpoint: listened endpoint
 * @param saved_model_file: saved model
 * @param party: initialized party
 */
// TODO: consider to use a factory class to concise the servers
void run_active_server_gbdt(const std::string &endpoint,
                            const std::string &saved_model_file,
                            const Party &party);

/**
 * run passive server to listen active party's requests
 *
 * @param saved_model_file: saved model
 * @param party: initialized party
 */
void run_passive_server_gbdt(const std::string &saved_model_file,
                             const Party &party);

#endif // FALCON_INCLUDE_FALCON_INFERENCE_SERVER_GBDT_INFERENCE_SERVICE_H_
