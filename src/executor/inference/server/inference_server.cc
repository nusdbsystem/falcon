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

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceLR;

// Logic and data behind the server's behavior.
class LRInferenceServiceImpl final : public InferenceLR::Service {
 public:
  explicit LRInferenceServiceImpl(
      const std::string& saved_model_file,
      const Party& party) {
    party_ = party;
  }

  Status Prediction(ServerContext* context, const PredictionRequest* request,
                    PredictionResponse* response) override {
    // parse the client request
    int sample_num = request->sample_num();
    response->set_sample_num(sample_num);

    // execute prediction for each sample id
    for (int i = 0; i < sample_num; i++) {
      int sample_id = request->sample_ids(i);

    }

    return Status::OK;
  }

 private:
  Party party_;
};

void RunServer(const std::string& endpoint,
    const std::string& saved_model_file,
    const Party& party) {
  LRInferenceServiceImpl service(saved_model_file, party);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(endpoint, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << endpoint << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}
