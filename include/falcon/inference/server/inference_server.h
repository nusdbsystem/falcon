//
// Created by wuyuncheng on 9/3/21.
//

#ifndef FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
#define FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_

#include <string>
#include <falcon/party/party.h>

void RunServer(const std::string& endpoint,
               const std::string& saved_model_file,
               const Party& party);

#endif //FALCON_SRC_EXECUTOR_INFERENCE_SERVER_INFERENCE_SERVER_H_
