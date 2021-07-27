//
// Created by wuyuncheng on 4/6/21.
//

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "../../include/message/inference/lr_grpc.grpc.pb.h"

#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/model/model_io.h>
#include <falcon/inference/server/lr_inference_service.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>

#include <glog/logging.h>
#include <Networking/ssl_sockets.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerCompletionQueue;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceService;

//// Logic and data behind the server's behavior.
//class LRInferenceServiceImpl final : public InferenceService::Service {
// public:
//  explicit LRInferenceServiceImpl(
//      const std::string& saved_model_file,
//      const Party& party) {
//    party_ = party;
//    // load_lr_model(saved_model_file, saved_lr_model_);
//    std::string saved_model_string;
//    load_pb_model_string(saved_model_string, saved_model_file);
//    deserialize_lr_model(saved_lr_model_, saved_model_string);
//  }
//
////  Status Prediction(ServerContext* context, const PredictionRequest* request,
////                    PredictionResponse* response) override {
////    std::cout << "Receive client's request" << std::endl;
////    LOG(INFO) << "Receive client's request";
////
////    // parse the client request
////    int sample_num = request->sample_num();
////    std::vector<int> batch_indexes;
////    // execute prediction for each sample id
////    for (int i = 0; i < sample_num; i++) {
////      batch_indexes.push_back(request->sample_ids(i));
////    }
////
////    std::cout << "Parse client's request finished" << std::endl;
////    LOG(INFO) << "Parse client's request finished";
////
////    // send batch indexes to passive parties
////    std::string batch_indexes_str;
////    serialize_int_array(batch_indexes, batch_indexes_str);
////    for (int i = 0; i < party_.party_num; i++) {
////      if (i != party_.party_id) {
////        party_.send_long_message(i, batch_indexes_str);
////      }
////    }
////
////    std::cout << "Broadcast client's batch requests to other parties" << std::endl;
////    LOG(INFO) << "Broadcast client's batch requests to other parties";
////
////    // local compute aggregation and receive from passive parties
////    // retrieve phe pub key and phe random
////    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
////    party_.getter_phe_pub_key(phe_pub_key);
////
////    // retrieve batch samples and encode (notice to use cur_batch_size
////    // instead of default batch size to avoid unexpected batch)
////    EncodedNumber* batch_phe_aggregation = new EncodedNumber[sample_num];
////    std::vector< std::vector<double> > batch_samples;
////    for (int i = 0; i < sample_num; i++) {
////      batch_samples.push_back(party_.getter_local_data()[batch_indexes[i]]);
////    }
////    EncodedNumber** encoded_batch_samples = new EncodedNumber*[sample_num];
////    for (int i = 0; i < sample_num; i++) {
////      encoded_batch_samples[i] = new EncodedNumber[saved_lr_model_.weight_size];
////    }
////    for (int i = 0; i < sample_num; i++) {
////      for (int j = 0; j < saved_lr_model_.weight_size; j++) {
////        encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
////                                               batch_samples[i][j], PHE_FIXED_POINT_PRECISION);
////      }
////    }
////
////    // compute local homomorphic aggregation
////    EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[sample_num];
////    djcs_t_aux_matrix_mult(phe_pub_key, party_.phe_random, local_batch_phe_aggregation,
////                           saved_lr_model_.local_weights, encoded_batch_samples, sample_num, saved_lr_model_.weight_size);
////
////    // every party sends the local aggregation to the active party
////    // copy self local aggregation results
////    for (int i = 0; i < sample_num; i++) {
////      batch_phe_aggregation[i] = local_batch_phe_aggregation[i];
////    }
////
////    std::cout << "Local phe aggregation finished" << std::endl;
////    LOG(INFO) << "Local phe aggregation finished";
////
////    // receive serialized string from other parties
////    // deserialize and sum to batch_phe_aggregation
////    for (int id = 0; id < party_.party_num; id++) {
////      if (id != party_.party_id) {
////        EncodedNumber* recv_batch_phe_aggregation = new EncodedNumber[sample_num];
////        std::string recv_local_aggregation_str;
////        party_.recv_long_message(id, recv_local_aggregation_str);
////        deserialize_encoded_number_array(recv_batch_phe_aggregation,
////                                         sample_num, recv_local_aggregation_str);
////        // homomorphic addition of the received aggregations
////        for (int i = 0; i < sample_num; i++) {
////          djcs_t_aux_ee_add(phe_pub_key,batch_phe_aggregation[i],
////                            batch_phe_aggregation[i], recv_batch_phe_aggregation[i]);
////        }
////        delete [] recv_batch_phe_aggregation;
////      }
////    }
////
////    std::cout << "Global phe aggregation finished" << std::endl;
////    LOG(INFO) << "Global phe aggregation finished";
////
////    // serialize and send the batch_phe_aggregation to other parties
////    std::string global_aggregation_str;
////    serialize_encoded_number_array(batch_phe_aggregation,
////                                   sample_num, global_aggregation_str);
////    for (int id = 0; id < party_.party_num; id++) {
////      if (id != party_.party_id) {
////        party_.send_long_message(id, global_aggregation_str);
////      }
////    }
////
////    std::cout << "Broad global phe aggregation result" << std::endl;
////    LOG(INFO) << "Broad global phe aggregation result";
////
////    // step 3: active party aggregates and call collaborative decryption
////    EncodedNumber* decrypted_aggregation = new EncodedNumber[sample_num];
////    party_.collaborative_decrypt(batch_phe_aggregation,
////                                 decrypted_aggregation,
////                                 sample_num,
////                                 ACTIVE_PARTY_ID);
////
////    std::cout << "Collaboratively decryption finished" << std::endl;
////    LOG(INFO) << "Collaboratively decryption finished";
////
////    // step 4: active party computes the logistic function and compare the accuracy
////    std::vector<double> labels;
////    std::vector< std::vector<double> > probabilities;
////    for (int i = 0; i < sample_num; i++) {
////      double t;
////      decrypted_aggregation[i].decode(t);
////      // std::cout << "t before logistic = " << t << std::endl;
////      t = 1.0 / (1 + exp(0 - t));
////      // std::cout << "t after logistic = " << t << std::endl;
////      std::vector<double> prob;
////      prob.push_back(t);
////      prob.push_back(1 - t);
////      t = t >= 0.5 ? 1 : 0;
////      // std::cout << "label = " << t << std::endl;
////      labels.push_back(t);
////      probabilities.push_back(prob);
////    }
////
////    std::cout << "Compute prediction finished" << std::endl;
////    LOG(INFO) << "Compute prediction finished";
////
////    // assemble response
////    response->set_sample_num(sample_num);
////    for (int i = 0; i < sample_num; i++) {
////      com::nus::dbsytem::falcon::v0::inference::PredictionOutput *output = response->add_outputs();
////      output->set_label(labels[i]);
////      // std::cout << "probabilities[0].size = " << probabilities[0].size() << std::endl;
////      for (int j = 0; j < probabilities[0].size(); j++) {
////        output->add_probabilities(probabilities[i][j]);
////      }
////    }
////
////    djcs_t_free_public_key(phe_pub_key);
////    for (int i = 0; i < sample_num; i++) {
////      delete [] encoded_batch_samples[i];
////    }
////    delete [] encoded_batch_samples;
////    delete [] local_batch_phe_aggregation;
////    delete [] batch_phe_aggregation;
////    delete [] decrypted_aggregation;
////
////    google::FlushLogFiles(google::INFO);
////    return Status::OK;
////  }
//
//  Status Prediction(ServerContext* context, const PredictionRequest* request,
//                    PredictionResponse* response) override {
//    std::cout << "Receive client's request" << std::endl;
//    LOG(INFO) << "Receive client's request";
//
//    // parse the client request
//    int sample_num = request->sample_num();
//    std::vector<int> batch_indexes;
//    // execute prediction for each sample id
//    for (int i = 0; i < sample_num; i++) {
//      batch_indexes.push_back(request->sample_ids(i));
//    }
//
//    std::cout << "Parse client's request finished" << std::endl;
//    LOG(INFO) << "Parse client's request finished";
//
//    // send batch indexes to passive parties
//    std::string batch_indexes_str;
//    serialize_int_array(batch_indexes, batch_indexes_str);
//    for (int i = 0; i < party_.party_num; i++) {
//      if (i != party_.party_id) {
//        party_.send_long_message(i, batch_indexes_str);
//      }
//    }
//
//    std::cout << "Broadcast client's batch requests to other parties" << std::endl;
//    LOG(INFO) << "Broadcast client's batch requests to other parties";
//
//    // local compute aggregation and receive from passive parties
//    // retrieve phe pub key and phe random
//    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
//    party_.getter_phe_pub_key(phe_pub_key);
//
//    // retrieve batch samples and encode (notice to use cur_batch_size
//    // instead of default batch size to avoid unexpected batch)
//    EncodedNumber* batch_phe_aggregation = new EncodedNumber[sample_num];
//    std::vector< std::vector<double> > batch_samples;
//    for (int i = 0; i < sample_num; i++) {
//      batch_samples.push_back(party_.getter_local_data()[batch_indexes[i]]);
//    }
//    EncodedNumber** encoded_batch_samples = new EncodedNumber*[sample_num];
//    for (int i = 0; i < sample_num; i++) {
//      encoded_batch_samples[i] = new EncodedNumber[saved_lr_model_.weight_size];
//    }
//    for (int i = 0; i < sample_num; i++) {
//      for (int j = 0; j < saved_lr_model_.weight_size; j++) {
//        encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
//                                               batch_samples[i][j], PHE_FIXED_POINT_PRECISION);
//      }
//    }
//
//    // compute local homomorphic aggregation
//    EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[sample_num];
//    djcs_t_aux_matrix_mult(phe_pub_key, party_.phe_random, local_batch_phe_aggregation,
//                           saved_lr_model_.local_weights, encoded_batch_samples, sample_num, saved_lr_model_.weight_size);
//
//    // every party sends the local aggregation to the active party
//    // copy self local aggregation results
//    for (int i = 0; i < sample_num; i++) {
//      batch_phe_aggregation[i] = local_batch_phe_aggregation[i];
//    }
//
//    std::cout << "Local phe aggregation finished" << std::endl;
//    LOG(INFO) << "Local phe aggregation finished";
//
//    // receive serialized string from other parties
//    // deserialize and sum to batch_phe_aggregation
//    for (int id = 0; id < party_.party_num; id++) {
//      if (id != party_.party_id) {
//        EncodedNumber* recv_batch_phe_aggregation = new EncodedNumber[sample_num];
//        std::string recv_local_aggregation_str;
//        party_.recv_long_message(id, recv_local_aggregation_str);
//        deserialize_encoded_number_array(recv_batch_phe_aggregation,
//                                         sample_num, recv_local_aggregation_str);
//        // homomorphic addition of the received aggregations
//        for (int i = 0; i < sample_num; i++) {
//          djcs_t_aux_ee_add(phe_pub_key,batch_phe_aggregation[i],
//                            batch_phe_aggregation[i], recv_batch_phe_aggregation[i]);
//        }
//        delete [] recv_batch_phe_aggregation;
//      }
//    }
//
//    std::cout << "Global phe aggregation finished" << std::endl;
//    LOG(INFO) << "Global phe aggregation finished";
//
//    // serialize and send the batch_phe_aggregation to other parties
//    std::string global_aggregation_str;
//    serialize_encoded_number_array(batch_phe_aggregation,
//                                   sample_num, global_aggregation_str);
//    for (int id = 0; id < party_.party_num; id++) {
//      if (id != party_.party_id) {
//        party_.send_long_message(id, global_aggregation_str);
//      }
//    }
//
//    std::cout << "Broad global phe aggregation result" << std::endl;
//    LOG(INFO) << "Broad global phe aggregation result";
//
//    // step 3: active party aggregates and call collaborative decryption
//    EncodedNumber* decrypted_aggregation = new EncodedNumber[sample_num];
//    party_.collaborative_decrypt(batch_phe_aggregation,
//                                 decrypted_aggregation,
//                                 sample_num,
//                                 ACTIVE_PARTY_ID);
//
//    std::cout << "Collaboratively decryption finished" << std::endl;
//    LOG(INFO) << "Collaboratively decryption finished";
//
//    // step 4: active party computes the logistic function and compare the accuracy
//    std::vector<double> labels;
//    std::vector< std::vector<double> > probabilities;
//    for (int i = 0; i < sample_num; i++) {
//      double t;
//      decrypted_aggregation[i].decode(t);
//      // std::cout << "t before logistic = " << t << std::endl;
//      t = 1.0 / (1 + exp(0 - t));
//      // std::cout << "t after logistic = " << t << std::endl;
//      std::vector<double> prob;
//      prob.push_back(t);
//      prob.push_back(1 - t);
//      t = t >= 0.5 ? 1 : 0;
//      // std::cout << "label = " << t << std::endl;
//      labels.push_back(t);
//      probabilities.push_back(prob);
//    }
//
//    std::cout << "Compute prediction finished" << std::endl;
//    LOG(INFO) << "Compute prediction finished";
//
//    // assemble response
//    response->set_sample_num(sample_num);
//    for (int i = 0; i < sample_num; i++) {
//      com::nus::dbsytem::falcon::v0::inference::PredictionOutput *output = response->add_outputs();
//      output->set_label(labels[i]);
//      // std::cout << "probabilities[0].size = " << probabilities[0].size() << std::endl;
//      for (int j = 0; j < probabilities[0].size(); j++) {
//        output->add_probabilities(probabilities[i][j]);
//      }
//    }
//
//    djcs_t_free_public_key(phe_pub_key);
//    for (int i = 0; i < sample_num; i++) {
//      delete [] encoded_batch_samples[i];
//    }
//    delete [] encoded_batch_samples;
//    delete [] local_batch_phe_aggregation;
//    delete [] batch_phe_aggregation;
//    delete [] decrypted_aggregation;
//
//    google::FlushLogFiles(google::INFO);
//    return Status::OK;
//  }
//
// private:
//  Party party_;
//  LogisticRegressionModel saved_lr_model_;
//};

class LrActiveServerImpl final {
 public:
  // LrActiveServerImpl(LRInferenceServiceImpl service) : service_(service) {}
  ~LrActiveServerImpl() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run(const std::string& endpoint, const std::string& saved_model_file,
           const Party& party) {
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(endpoint, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << endpoint << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs(saved_model_file, party);
  }

 private:
  // Class encompasing the state and logic needed to serve a request.
  class CallData {
   public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(InferenceService::AsyncService* service, ServerCompletionQueue* cq,
             const std::string& saved_model_file,
             const Party& party)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // Invoke the serving logic right away.
      Proceed(saved_model_file, party);
    }

    void Proceed(const std::string& saved_model_file,
                 const Party& party_) {
      if (status_ == CREATE) {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

        // As part of the initial CREATE state, we *request* that the system
        // start processing SayHello requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different CallData
        // instances can serve different requests concurrently), in this case
        // the memory address of this CallData instance.
        service_->RequestPrediction(&ctx_, &request_, &responder_, cq_, cq_,
            this);
      } else if (status_ == PROCESS) {
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
        new CallData(service_, cq_, saved_model_file, party_);

        // The actual processing. Add process logic here
        LogisticRegressionModel saved_lr_model_;
        std::string saved_model_string;
        load_pb_model_string(saved_model_string, saved_model_file);
        deserialize_lr_model(saved_lr_model_, saved_model_string);

        std::cout << "Receive client's request" << std::endl;
        LOG(INFO) << "Receive client's request";
        // parse the client request
        int sample_num = request_.sample_num();
        std::vector<int> batch_indexes;
        // execute prediction for each sample id
        for (int i = 0; i < sample_num; i++) {
          batch_indexes.push_back(request_.sample_ids(i));
        }

        std::cout << "Parse client's request finished" << std::endl;
        LOG(INFO) << "Parse client's request finished";

        // send batch indexes to passive parties
        std::string batch_indexes_str;
        serialize_int_array(batch_indexes, batch_indexes_str);
        for (int i = 0; i < party_.party_num; i++) {
          if (i != party_.party_id) {
            party_.send_long_message(i, batch_indexes_str);
          }
        }


        // Here put the whole setup socket code together, as using a function call
        // would result in a problem when deleting the created sockets
        // setup connections from this party to each spdz party socket
        std::vector<ssl_socket*> mpc_sockets(party_.party_num);
        vector<int> plain_sockets(party_.party_num);
        // ssl_ctx ctx(mpc_player_path, "C" + to_string(party_id));
        ssl_ctx ctx("C" + to_string(party_.party_id));
        // std::cout << "correct init ctx" << std::endl;
        ssl_service io_service;
        octetStream specification;
        std::cout << "begin connect to spdz parties" << std::endl;
        std::cout << "party_num = " << party_.party_num << std::endl;

        std::cout << "Broadcast client's batch requests to other parties" << std::endl;
        LOG(INFO) << "Broadcast client's batch requests to other parties";

        // retrieve batch samples and encode (notice to use cur_batch_size
        // instead of default batch size to avoid unexpected batch)
        EncodedNumber* predicted_labels = new EncodedNumber[sample_num];
        std::vector< std::vector<double> > batch_samples;
        for (int i = 0; i < sample_num; i++) {
          batch_samples.push_back(party_.getter_local_data()[batch_indexes[i]]);
        }

        saved_lr_model_.predict(const_cast<Party &>(party_), batch_samples, sample_num, predicted_labels);

        // step 3: active party aggregates and call collaborative decryption
        EncodedNumber* decrypted_labels = new EncodedNumber[sample_num];
        party_.collaborative_decrypt(predicted_labels,
                                     decrypted_labels,
                                     sample_num,
                                     ACTIVE_PARTY_ID);

        std::cout << "Collaboratively decryption finished" << std::endl;
        LOG(INFO) << "Collaboratively decryption finished";

        // step 4: active party computes the logistic function and compare the accuracy
        std::vector<double> labels;
        std::vector< std::vector<double> > probabilities;
        for (int i = 0; i < sample_num; i++) {
          double t;
          decrypted_labels[i].decode(t);
          // std::cout << "t before logistic = " << t << std::endl;
          // t = 1.0 / (1 + exp(0 - t));
          // std::cout << "t after logistic = " << t << std::endl;
          std::vector<double> prob;
          prob.push_back(t);
          prob.push_back(1 - t);
          t = t >= 0.5 ? 1 : 0;
          // std::cout << "label = " << t << std::endl;
          labels.push_back(t);
          probabilities.push_back(prob);
        }

        std::cout << "Compute prediction finished" << std::endl;
        LOG(INFO) << "Compute prediction finished";

        // assemble response
        reply_.set_sample_num(sample_num);
        for (int i = 0; i < sample_num; i++) {
          com::nus::dbsytem::falcon::v0::inference::PredictionOutput *output = reply_.add_outputs();
          output->set_label(labels[i]);
          // std::cout << "probabilities[0].size = " << probabilities[0].size() << std::endl;
          for (int j = 0; j < probabilities[0].size(); j++) {
            output->add_probabilities(probabilities[i][j]);
          }
        }

        delete [] predicted_labels;
        delete [] decrypted_labels;
        google::FlushLogFiles(google::INFO);

        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
        delete this;
      }
    }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    InferenceService::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    PredictionRequest request_;
    // What we send back to the client.
    PredictionResponse reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<PredictionResponse> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  // This can be run in multiple threads if needed.
  void HandleRpcs(const std::string& saved_model_file,
                  const Party& party) {
    // Spawn a new CallData instance to serve new clients.
    new CallData(&service_, cq_.get(), saved_model_file, party);
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed(saved_model_file, party);
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  InferenceService::AsyncService service_;
  std::unique_ptr<Server> server_;
};

void run_active_server_lr(const std::string& endpoint,
    const std::string& saved_model_file,
    const Party& party) {
//  LRInferenceServiceImpl service(saved_model_file, party);
//
//  grpc::EnableDefaultHealthCheckService(true);
//  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
//  ServerBuilder builder;
//  // Listen on the given address without any authentication mechanism.
//  builder.AddListeningPort(endpoint, grpc::InsecureServerCredentials());
//  // Register "service" as the instance through which we'll communicate with
//  // clients. In this case it corresponds to an *synchronous* service.
//  builder.RegisterService(&service);
//  // Finally assemble the server.
//  std::unique_ptr<Server> server(builder.BuildAndStart());
//  std::cout << "Server listening on " << endpoint << std::endl;
//
//  // Wait for the server to shutdown. Note that some other thread must be
//  // responsible for shutting down the server for this call to ever return.
//  server->Wait();
  // LRInferenceServiceImpl service(saved_model_file, party);
  LrActiveServerImpl lr_active_server;
  lr_active_server.Run(endpoint, saved_model_file, party);
}

void run_passive_server_lr(const std::string& saved_model_file,
    const Party& party) {
  LogisticRegressionModel saved_lr_model;
  // load_lr_model(saved_model_file, saved_lr_model);
  std::string saved_model_string;
  load_pb_model_string(saved_model_string, saved_model_file);
  deserialize_lr_model(saved_lr_model, saved_model_string);

//  // keep listening requests from the active party
//  while (true) {
//    std::cout << "listen active party's request" << std::endl;
//    // receive sample id from active party
//    std::string recv_batch_indexes_str;
//    party.recv_long_message(ACTIVE_PARTY_ID, recv_batch_indexes_str);
//    std::vector<int> batch_indexes;
//    deserialize_int_array(batch_indexes, recv_batch_indexes_str);
//
//    std::cout << "Received active party's batch indexes" << std::endl;
//    LOG(INFO) << "Received active party's batch indexes";
//
//    // retrieve phe pub key and phe random
//    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
//    party.getter_phe_pub_key(phe_pub_key);
//
//    // retrieve batch samples and encode (notice to use cur_batch_size
//    // instead of default batch size to avoid unexpected batch)
//    int cur_batch_size = batch_indexes.size();
//    EncodedNumber* batch_phe_aggregation = new EncodedNumber[cur_batch_size];
//    std::vector< std::vector<double> > batch_samples;
//    for (int i = 0; i < cur_batch_size; i++) {
//      batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
//    }
//    EncodedNumber** encoded_batch_samples = new EncodedNumber*[cur_batch_size];
//    for (int i = 0; i < cur_batch_size; i++) {
//      encoded_batch_samples[i] = new EncodedNumber[saved_lr_model.weight_size];
//    }
//    for (int i = 0; i < cur_batch_size; i++) {
//      for (int j = 0; j < saved_lr_model.weight_size; j++) {
//        encoded_batch_samples[i][j].set_double(phe_pub_key->n[0],
//                                               batch_samples[i][j], PHE_FIXED_POINT_PRECISION);
//      }
//    }
//
//    // compute local homomorphic aggregation
//    EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
//    djcs_t_aux_matrix_mult(phe_pub_key, party.phe_random, local_batch_phe_aggregation,
//                           saved_lr_model.local_weights, encoded_batch_samples, cur_batch_size, saved_lr_model.weight_size);
//
//    std::cout << "Local phe aggregation finished" << std::endl;
//    LOG(INFO) << "Local phe aggregation finished";
//
//    // serialize local batch aggregation and send to active party
//    std::string local_aggregation_str;
//    serialize_encoded_number_array(local_batch_phe_aggregation,
//                                   cur_batch_size, local_aggregation_str);
//    party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);
//
//    std::cout << "Send local phe aggregation string to active party" << std::endl;
//    LOG(INFO) << "Send local phe aggregation string to active party";
//
//    // receive the global batch aggregation from the active party
//    std::string recv_global_aggregation_str;
//    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
//    deserialize_encoded_number_array(batch_phe_aggregation,
//                                     cur_batch_size, recv_global_aggregation_str);
//
//    std::cout << "Received global phe aggregation from active party" << std::endl;
//    LOG(INFO) << "Received global phe aggregation from active party";
//
//    // step 3: active party aggregates and call collaborative decryption
//    EncodedNumber* decrypted_aggregation = new EncodedNumber[cur_batch_size];
//    party.collaborative_decrypt(batch_phe_aggregation,
//                                decrypted_aggregation,
//                                cur_batch_size,
//                                ACTIVE_PARTY_ID);
//
//    std::cout << "Collaboratively decryption finished" << std::endl;
//    LOG(INFO) << "Collaboratively decryption finished";
//
//    djcs_t_free_public_key(phe_pub_key);
//    for (int i = 0; i < cur_batch_size; i++) {
//      delete [] encoded_batch_samples[i];
//    }
//    delete [] encoded_batch_samples;
//    delete [] local_batch_phe_aggregation;
//    delete [] batch_phe_aggregation;
//    delete [] decrypted_aggregation;
//
//    google::FlushLogFiles(google::INFO);
//  }

// keep listening requests from the active party
  while (true) {
    std::cout << "listen active party's request" << std::endl;
    // receive sample id from active party
    std::string recv_batch_indexes_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_batch_indexes_str);
    std::vector<int> batch_indexes;
    deserialize_int_array(batch_indexes, recv_batch_indexes_str);

    std::cout << "Received active party's batch indexes" << std::endl;
    LOG(INFO) << "Received active party's batch indexes";

    // retrieve batch samples and encode (notice to use cur_batch_size
    // instead of default batch size to avoid unexpected batch)
    int cur_batch_size = batch_indexes.size();
    EncodedNumber *predicted_labels = new EncodedNumber[cur_batch_size];
    std::vector< std::vector<double> > batch_samples;
    for (int i = 0; i < cur_batch_size; i++) {
      batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
    }
    saved_lr_model.predict(const_cast<Party &>(party), batch_samples, cur_batch_size, predicted_labels);

    // step 3: active party aggregates and call collaborative decryption
    EncodedNumber* decrypted_labels = new EncodedNumber[cur_batch_size];
    party.collaborative_decrypt(predicted_labels,
                                decrypted_labels,
                                cur_batch_size,
                                ACTIVE_PARTY_ID);

    std::cout << "Collaboratively decryption finished" << std::endl;
    LOG(INFO) << "Collaboratively decryption finished";

    delete [] predicted_labels;
    delete [] decrypted_labels;

    google::FlushLogFiles(google::INFO);
  }
}