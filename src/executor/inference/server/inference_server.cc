//
// Created by wuyuncheng on 9/3/21.
//

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "../../include/message/inference/lr_grpc.grpc.pb.h"

#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/model/model_io.h>
#include <falcon/inference/server/inference_server.h>
#include <falcon/inference/server/lr_inference_service.h>
#include <falcon/inference/server/dt_inference_service.h>
#include <falcon/inference/server/rf_inference_service.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceService;

void run_active_server(const std::string& endpoint,
    const std::string& saved_model_file,
    const Party& party,
    falcon::AlgorithmName algorithm_name) {
  switch(algorithm_name) {
    case falcon::LR:
      run_active_server_lr(endpoint, saved_model_file, party);
      break;
    case falcon::DT:
      run_active_server_dt(endpoint, saved_model_file, party);
      break;
    case falcon::RF:
      run_active_server_rf(endpoint, saved_model_file, party);
      break;
    default:
      run_active_server_lr(endpoint, saved_model_file, party);
      break;
  }
}

void run_passive_server(const std::string& saved_model_file,
    const Party& party,
    falcon::AlgorithmName algorithm_name) {
  switch(algorithm_name) {
    case falcon::LR:
      run_passive_server_lr(saved_model_file, party);
      break;
    case falcon::DT:
      run_passive_server_dt(saved_model_file, party);
      break;
    case falcon::RF:
      run_passive_server_rf(saved_model_file, party);
      break;
    default:
      run_passive_server_lr(saved_model_file, party);
      break;
  }
}