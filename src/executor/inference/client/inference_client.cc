//
// Created by wuyuncheng on 8/3/21.
//

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "../../include/message/inference/lr_grpc.grpc.pb.h"

#include <falcon/common.h>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientAsyncResponseReader;
using grpc::CompletionQueue;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceService;

class InferenceClient {
 public:
  InferenceClient(std::shared_ptr<Channel> channel)
      : stub_(InferenceService::NewStub(channel)) {}

  // Assembles the client's payload and sends it to the server.
  void Prediction(int sample_num, int * sample_ids) {
    // Data we are sending to the server.
    PredictionRequest request;
    request.set_sample_num(sample_num);
    for (int i = 0; i < sample_num; i++) {
      request.add_sample_ids(sample_ids[i]);
    }

    // Call object to store rpc data
    AsyncClientCall* call = new AsyncClientCall;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader =
        stub_->PrepareAsyncPrediction(&call->context, request, &cq_);

    // StartCall initiates the RPC call
    call->response_reader->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call
    // object.
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }

  // Loop while listening for completed responses.
  // Prints out the response from the server.
  void AsyncCompleteRpc(int sample_num, int * sample_ids) {
    void* got_tag;
    bool ok = false;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {
      // The tag in this example is the memory location of the call object
      AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

      // Verify that the request was completed successfully. Note that "ok"
      // corresponds solely to the request for updates introduced by Finish().
      GPR_ASSERT(ok);

      // Act upon its status.
      if (call->status.ok()) {
        int received_sample_num = call->reply.sample_num();
        if (received_sample_num != sample_num) {
          std::cout << "Received prediction number does not match\n";
          std::cout << "Prediction failed";
        } else {
          for (int i = 0; i < received_sample_num; i++) {
            double label = call->reply.outputs(i).label();
            std::cout << "Sample " << sample_ids[i] << " predicted label = " << label << ", ";
            std::cout << "Probabilities = [ ";
            for (int j = 0; j < call->reply.outputs(0).probabilities_size(); j++) {
              std::cout << call->reply.outputs(i).probabilities(j) << " ";
            }
            std::cout << "]" << std::endl;
          }
          std::cout << "Prediction success";
        }
      } else {
        std::cout << call->status.error_code() << ": " << call->status.error_message() << std::endl;
        std::cout << "RPC failed";
      }

      // Once we're complete, deallocate the call object.
      delete call;
    }
  }

 private:
  // struct for keeping state and data information
  struct AsyncClientCall {
    // Container for the data we expect from the server.
    PredictionResponse reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // Storage for the status of the RPC upon completion.
    Status status;

    std::unique_ptr<ClientAsyncResponseReader<PredictionResponse>> response_reader;
  };

  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<InferenceService::Stub> stub_;

  // The producer-consumer queue we use to communicate asynchronously with the
  // gRPC runtime.
  CompletionQueue cq_;


//
//  // Assembles the client's payload, sends it and presents the response back
//  // from the server.
//  std::string Prediction(int sample_num, int * sample_ids) {
//    // Data we are sending to the server.
//    PredictionRequest request;
//    request.set_sample_num(sample_num);
//    for (int i = 0; i < sample_num; i++) {
//      request.add_sample_ids(sample_ids[i]);
//    }
//
//    // Container for the data we expect from the server.
//    PredictionResponse response;
//
//    // The producer-consumer queue we use to communicate asynchronously with the
//    // gRPC runtime.
//    CompletionQueue cq;
//
//    // Context for the client. It could be used to convey extra information to
//    // the server and/or tweak certain RPC behaviors.
//    ClientContext context;
//
//    // The actual RPC.
//    // Status status = stub_->Prediction(&context, request, &response);
//    Status status;
//
//    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
//    // an instance to store in "call" but does not actually start the RPC
//    // Because we are using the asynchronous API, we need to hold on to
//    // the "call" instance in order to get updates on the ongoing RPC.
//    std::unique_ptr<ClientAsyncResponseReader<PredictionResponse> > rpc(
//        stub_->Prediction(&context, request, &response));
//
//    // StartCall initiates the RPC call
//    rpc->StartCall();
//
//    // Request that, upon completion of the RPC, "reply" be updated with the
//    // server's response; "status" with the indication of whether the operation
//    // was successful. Tag the request with the integer 1.
//    rpc->Finish(&response, &status, (void*)1);
//    void* got_tag;
//    bool ok = false;
//    // Block until the next result is available in the completion queue "cq".
//    // The return value of Next should always be checked. This return value
//    // tells us whether there is any kind of event or the cq_ is shutting down.
//    GPR_ASSERT(cq.Next(&got_tag, &ok));
//
//    // Verify that the result from "cq" corresponds, by its tag, our previous
//    // request.
//    GPR_ASSERT(got_tag == (void*)1);
//    // ... and that the request was completed successfully. Note that "ok"
//    // corresponds solely to the request for updates introduced by Finish().
//    GPR_ASSERT(ok);
//
//    // Act upon its status.
//    if (status.ok()) {
//      int received_sample_num = response.sample_num();
//      if (received_sample_num != sample_num) {
//        std::cout << "Received prediction number does not match\n";
//        return "Prediction failed";
//      } else {
//        for (int i = 0; i < received_sample_num; i++) {
//          double label = response.outputs(i).label();
//          std::cout << "Sample " << sample_ids[i] << " predicted label = " << label << ", ";
//          std::cout << "Probabilities = [ ";
//          for (int j = 0; j < response.outputs(0).probabilities_size(); j++) {
//            std::cout << response.outputs(i).probabilities(j) << " ";
//          }
//          std::cout << "]" << std::endl;
//        }
//        return "Prediction success";
//      }
//    } else {
//      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
//      return "RPC failed";
//    }
//  }
//
// private:
//  std::unique_ptr<InferenceService::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  // The parameters include number of samples, sample ids, endpoint
  int sample_num = std::stoi(argv[1]);
  std::cout << "sample num = " << sample_num << std::endl;
  int * sample_ids = new int[sample_num];
  std::string endpoint;
  for (int i = 0; i < sample_num; i++) {
    sample_ids[i] = std::stoi(argv[i + 2]);
    std::cout << "sample ids [" << i << "] = " << sample_ids[i] << std::endl;
  }
  if (argv[sample_num + 2]) {
    endpoint = argv[sample_num + 2];
  } else {
    endpoint = DEFAULT_INFERENCE_ENDPOINT;
  }

  std::cout << "endpoint = " << endpoint << std::endl;

  InferenceClient client(grpc::CreateChannel(
      endpoint, grpc::InsecureChannelCredentials()));

  // Spawn reader thread that loops indefinitely
  std::thread thread_ = std::thread(&InferenceClient::AsyncCompleteRpc, &client, sample_num, sample_ids);

  client.Prediction(sample_num, sample_ids);
  thread_.join();  // blocks forever

  delete [] sample_ids;
  return 0;
}
