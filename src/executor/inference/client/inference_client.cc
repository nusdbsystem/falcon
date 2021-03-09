//
// Created by wuyuncheng on 8/3/21.
//

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "../../include/message/inference/lr_grpc.grpc.pb.h"

#include <falcon/common.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceLR;

class LRInferenceClient {
 public:
  LRInferenceClient(std::shared_ptr<Channel> channel)
      : stub_(InferenceLR::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string Prediction(int sample_num, int * sample_ids) {
    // Data we are sending to the server.
    PredictionRequest request;
    request.set_sample_num(sample_num);
    for (int i = 0; i < sample_num; i++) {
      request.set_sample_ids(i, sample_ids[i]);
    }

    // Container for the data we expect from the server.
    PredictionResponse response;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Prediction(&context, request, &response);

    // Act upon its status.
    if (status.ok()) {
      int received_sample_num = response.sample_num();
      if (received_sample_num != sample_num) {
        std::cout << "Received prediction number does not match\n";
        return "Prediction failed";
      } else {
        for (int i = 0; i < received_sample_num; i++) {
          float label = response.outputs(i).label();
          std::cout << "Sample " << sample_ids[i] << " predicted label = " << label << ", ";
          std::cout << "Probabilities = [ ";
          for (int j = 0; j < response.outputs_size(); j++) {
            std::cout << response.outputs(i).probabilities(j) << " ";
          }
          std::cout << "]" << std::endl;
        }
        return "Prediction success";
      }
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<InferenceLR::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  // The parameters include number of samples, sample ids, endpoint
  int sample_num = std::stoi(argv[1]);
  int * sample_ids = new int[sample_num];
  std::string endpoint;
  for (int i = 0; i < sample_num; i++) {
    sample_ids[i] = std::stoi(argv[i + 2]);
  }
  if (argv[sample_num + 2]) {
    endpoint = argv[sample_num + 2];
  } else {
    endpoint = DEFAULT_INFERENCE_ENDPOINT;
  }

  LRInferenceClient client(grpc::CreateChannel(
      endpoint, grpc::InsecureChannelCredentials()));
  std::string response_status = client.Prediction(sample_num, sample_ids);
  std::cout << "Response received: " << response_status << std::endl;

  delete [] sample_ids;
  return 0;
}
