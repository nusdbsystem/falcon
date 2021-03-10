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
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>

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
    weight_size_ = party.getter_feature_num();
    local_weights_ = new EncodedNumber[weight_size_];
    load_lr_model(saved_model_file, weight_size_, local_weights_);
  }

  Status Prediction(ServerContext* context, const PredictionRequest* request,
                    PredictionResponse* response) override {
    std::cout << "Receive client's request" << std::endl;
    LOG(INFO) << "Receive client's request";

    // parse the client request
    int sample_num = request->sample_num();
    std::vector<int> batch_indexes;
    // execute prediction for each sample id
    for (int i = 0; i < sample_num; i++) {
      batch_indexes.push_back(request->sample_ids(i));
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

    std::cout << "Broadcast client's batch requests to other parties" << std::endl;
    LOG(INFO) << "Broadcast client's batch requests to other parties";

    // local compute aggregation and receive from passive parties
    // retrieve phe pub key and phe random
    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
    hcs_random* phe_random = hcs_init_random();
    party_.getter_phe_pub_key(phe_pub_key);
    party_.getter_phe_random(phe_random);

    // retrieve batch samples and encode (notice to use cur_batch_size
    // instead of default batch size to avoid unexpected batch)
    EncodedNumber* batch_phe_aggregation = new EncodedNumber[sample_num];
    std::vector< std::vector<float> > batch_samples;
    for (int i = 0; i < sample_num; i++) {
      batch_samples.push_back(party_.getter_local_data()[batch_indexes[i]]);
    }
    EncodedNumber** encoded_batch_samples = new EncodedNumber*[sample_num];
    for (int i = 0; i < sample_num; i++) {
      encoded_batch_samples[i] = new EncodedNumber[weight_size_];
    }
    for (int i = 0; i < sample_num; i++) {
      for (int j = 0; j < weight_size_; j++) {
        encoded_batch_samples[i][j].set_float(phe_pub_key->n[0],
            batch_samples[i][j], PHE_FIXED_POINT_PRECISION);
      }
    }

    // compute local homomorphic aggregation
    EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[sample_num];
    djcs_t_aux_matrix_mult(phe_pub_key, phe_random, local_batch_phe_aggregation,
        local_weights_, encoded_batch_samples, sample_num, weight_size_);

    // every party sends the local aggregation to the active party
    // copy self local aggregation results
    for (int i = 0; i < sample_num; i++) {
      batch_phe_aggregation[i] = local_batch_phe_aggregation[i];
    }

    std::cout << "Local phe aggregation finished" << std::endl;
    LOG(INFO) << "Local phe aggregation finished";

    // receive serialized string from other parties
    // deserialize and sum to batch_phe_aggregation
    for (int id = 0; id < party_.party_num; id++) {
      if (id != party_.party_id) {
        EncodedNumber* recv_batch_phe_aggregation = new EncodedNumber[sample_num];
        std::string recv_local_aggregation_str;
        party_.recv_long_message(id, recv_local_aggregation_str);
        deserialize_encoded_number_array(recv_batch_phe_aggregation,
            sample_num, recv_local_aggregation_str);
        // homomorphic addition of the received aggregations
        for (int i = 0; i < sample_num; i++) {
          djcs_t_aux_ee_add(phe_pub_key,batch_phe_aggregation[i],
              batch_phe_aggregation[i], recv_batch_phe_aggregation[i]);
        }
        delete [] recv_batch_phe_aggregation;
      }
    }

    std::cout << "Global phe aggregation finished" << std::endl;
    LOG(INFO) << "Global phe aggregation finished";

    // serialize and send the batch_phe_aggregation to other parties
    std::string global_aggregation_str;
    serialize_encoded_number_array(batch_phe_aggregation,
        sample_num, global_aggregation_str);
    for (int id = 0; id < party_.party_num; id++) {
      if (id != party_.party_id) {
        party_.send_long_message(id, global_aggregation_str);
      }
    }

    std::cout << "Broad global phe aggregation result" << std::endl;
    LOG(INFO) << "Broad global phe aggregation result";

    // step 3: active party aggregates and call collaborative decryption
    EncodedNumber* decrypted_aggregation = new EncodedNumber[sample_num];
    party_.collaborative_decrypt(batch_phe_aggregation,
                                decrypted_aggregation,
                                sample_num,
                                ACTIVE_PARTY_ID);

    std::cout << "Collaboratively decryption finished" << std::endl;
    LOG(INFO) << "Collaboratively decryption finished";

    // step 4: active party computes the logistic function and compare the accuracy
    std::vector<float> labels;
    std::vector< std::vector<float> > probabilities;
    for (int i = 0; i < sample_num; i++) {
      float t;
      decrypted_aggregation[i].decode(t);
      // std::cout << "t before logistic = " << t << std::endl;
      t = 1.0 / (1 + exp(0 - t));
      // std::cout << "t after logistic = " << t << std::endl;
      std::vector<float> prob;
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
    response->set_sample_num(sample_num);
    for (int i = 0; i < sample_num; i++) {
      com::nus::dbsytem::falcon::v0::inference::PredictionOutput *output = response->add_outputs();
      output->set_label(labels[i]);
      // std::cout << "probabilities[0].size = " << probabilities[0].size() << std::endl;
      for (int j = 0; j < probabilities[0].size(); j++) {
        output->add_probabilities(probabilities[i][j]);
      }
    }

    djcs_t_free_public_key(phe_pub_key);
    hcs_free_random(phe_random);
    for (int i = 0; i < sample_num; i++) {
      delete [] encoded_batch_samples[i];
    }
    delete [] encoded_batch_samples;
    delete [] local_batch_phe_aggregation;
    delete [] batch_phe_aggregation;
    delete [] decrypted_aggregation;

    google::FlushLogFiles(google::INFO);
    return Status::OK;
  }

 private:
  Party party_;
  int weight_size_;
  EncodedNumber* local_weights_;
};

void RunActiveServerLR(const std::string& endpoint,
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

void RunPassiveServerLR(std::string saved_model_file, Party party) {
  int weight_size = party.getter_feature_num();
  EncodedNumber* local_weights = new EncodedNumber[weight_size];
  load_lr_model(saved_model_file, weight_size, local_weights);

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

    // retrieve phe pub key and phe random
    djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
    hcs_random* phe_random = hcs_init_random();
    party.getter_phe_pub_key(phe_pub_key);
    party.getter_phe_random(phe_random);

    // retrieve batch samples and encode (notice to use cur_batch_size
    // instead of default batch size to avoid unexpected batch)
    int cur_batch_size = batch_indexes.size();
    EncodedNumber* batch_phe_aggregation = new EncodedNumber[cur_batch_size];
    std::vector< std::vector<float> > batch_samples;
    for (int i = 0; i < cur_batch_size; i++) {
      batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
    }
    EncodedNumber** encoded_batch_samples = new EncodedNumber*[cur_batch_size];
    for (int i = 0; i < cur_batch_size; i++) {
      encoded_batch_samples[i] = new EncodedNumber[weight_size];
    }
    for (int i = 0; i < cur_batch_size; i++) {
      for (int j = 0; j < weight_size; j++) {
        encoded_batch_samples[i][j].set_float(phe_pub_key->n[0],
            batch_samples[i][j], PHE_FIXED_POINT_PRECISION);
      }
    }

    // compute local homomorphic aggregation
    EncodedNumber* local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
    djcs_t_aux_matrix_mult(phe_pub_key, phe_random, local_batch_phe_aggregation,
                           local_weights, encoded_batch_samples, cur_batch_size, weight_size);

    std::cout << "Local phe aggregation finished" << std::endl;
    LOG(INFO) << "Local phe aggregation finished";

    // serialize local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_batch_phe_aggregation,
                                   cur_batch_size, local_aggregation_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);

    std::cout << "Send local phe aggregation string to active party" << std::endl;
    LOG(INFO) << "Send local phe aggregation string to active party";

    // receive the global batch aggregation from the active party
    std::string recv_global_aggregation_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
    deserialize_encoded_number_array(batch_phe_aggregation,
        cur_batch_size, recv_global_aggregation_str);

    std::cout << "Received global phe aggregation from active party" << std::endl;
    LOG(INFO) << "Received global phe aggregation from active party";

    // step 3: active party aggregates and call collaborative decryption
    EncodedNumber* decrypted_aggregation = new EncodedNumber[cur_batch_size];
    party.collaborative_decrypt(batch_phe_aggregation,
                                decrypted_aggregation,
                                cur_batch_size,
                                ACTIVE_PARTY_ID);

    std::cout << "Collaboratively decryption finished" << std::endl;
    LOG(INFO) << "Collaboratively decryption finished";

    djcs_t_free_public_key(phe_pub_key);
    hcs_free_random(phe_random);
    for (int i = 0; i < cur_batch_size; i++) {
      delete [] encoded_batch_samples[i];
    }
    delete [] encoded_batch_samples;
    delete [] local_batch_phe_aggregation;
    delete [] batch_phe_aggregation;
    delete [] decrypted_aggregation;

    google::FlushLogFiles(google::INFO);
  }
}