//
// Created by wuyuncheng on 4/6/21.
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
#include <falcon/inference/server/dt_inference_service.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceService;

// Logic and data behind the server's behavior.
class DTInferenceServiceImpl final : public InferenceService::Service {
 public:
  explicit DTInferenceServiceImpl(
      const std::string& saved_model_file,
      const Party& party) {
    party_ = party;
    load_dt_model(saved_model_file, saved_tree_model_);
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
    party_.getter_phe_pub_key(phe_pub_key);

    // retrieve batch samples and encode (notice to use cur_batch_size
    // instead of default batch size to avoid unexpected batch)
    std::vector< std::vector<double> > batch_samples;
    for (int i = 0; i < sample_num; i++) {
      batch_samples.push_back(party_.getter_local_data()[batch_indexes[i]]);
    }

    // step 1: organize the leaf label vector, compute the map
    LOG(INFO) << "Tree internal node num = " << saved_tree_model_.internal_node_num;
    EncodedNumber *label_vector = new EncodedNumber[saved_tree_model_.internal_node_num + 1];
    std::map<int, int> node_index_2_leaf_index_map;
    int leaf_cur_index = 0;
    for (int i = 0; i < pow(2, saved_tree_model_.max_depth + 1) - 1; i++) {
      if (saved_tree_model_.nodes[i].node_type == falcon::LEAF) {
        node_index_2_leaf_index_map.insert(std::make_pair(i, leaf_cur_index));
        label_vector[leaf_cur_index] = saved_tree_model_.nodes[i].label;  // record leaf label vector
        leaf_cur_index ++;
      }
    }

    // record the ciphertext of the label
    EncodedNumber *encrypted_aggregation = new EncodedNumber[sample_num];
    // for each sample
    for (int i = 0; i < sample_num; i++) {
      // compute binary vector for the current sample
      std::vector<int> binary_vector = saved_tree_model_.comp_predict_vector(batch_samples[i], node_index_2_leaf_index_map);
      EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
      EncodedNumber *updated_label_vector = new EncodedNumber[binary_vector.size()];

      // the super client update the last, and aggregate before calling share decryption
      std::string final_recv_s;
      party_.recv_long_message(party_.party_id + 1, final_recv_s);
      deserialize_encoded_number_array(updated_label_vector, binary_vector.size(), final_recv_s);
      for (int j = 0; j < binary_vector.size(); j++) {
        encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
        djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
      }

      encrypted_aggregation[i].set_double(phe_pub_key->n[0], 0, PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party_.phe_random,
                         encrypted_aggregation[i], encrypted_aggregation[i]);
      for (int j = 0; j < binary_vector.size(); j++) {
        djcs_t_aux_ee_add(phe_pub_key, encrypted_aggregation[i],
                          encrypted_aggregation[i], updated_label_vector[j]);
      }

      delete [] encoded_binary_vector;
      delete [] updated_label_vector;
    }

    std::string s;
    serialize_encoded_number_array(encrypted_aggregation, sample_num, s);
    for (int x = 0; x < party_.party_num; x++) {
      if (x != party_.party_id) {
        party_.send_long_message(x, s);
      }
    }

    // step 3: active party aggregates and call collaborative decryption
    EncodedNumber* decrypted_aggregation = new EncodedNumber[sample_num];
    party_.collaborative_decrypt(encrypted_aggregation,
                                 decrypted_aggregation,
                                 sample_num,
                                 ACTIVE_PARTY_ID);

    std::cout << "Collaboratively decryption finished" << std::endl;
    LOG(INFO) << "Collaboratively decryption finished";

    // step 4: active party computes the label
    std::vector<double> labels;
    std::vector< std::vector<double> > probabilities;
    for (int i = 0; i < sample_num; i++) {
      double t;
      decrypted_aggregation[i].decode(t);
      labels.push_back(t);
      std::vector<double> prob;
      if (saved_tree_model_.type == falcon::CLASSIFICATION) {
        // TODO: record the detailed probability, here assume 1.0
        for (int k = 0; k < saved_tree_model_.class_num; k++) {
          if (t == k) {
            prob.push_back(POSITIVE_PROBABILITY);
          } else {
            prob.push_back(ZERO_PROBABILITY);
          }
        }
        probabilities.push_back(prob);
      } else {
        prob.push_back(POSITIVE_PROBABILITY);
      }
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
    delete [] encrypted_aggregation;
    delete [] decrypted_aggregation;

    google::FlushLogFiles(google::INFO);
    return Status::OK;
  }

 private:
  Party party_;
  Tree saved_tree_model_;
};

void run_active_server_dt(const std::string& endpoint,
                          const std::string& saved_model_file,
                          const Party& party) {
  DTInferenceServiceImpl service(saved_model_file, party);

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

void run_passive_server_dt(const std::string& saved_model_file,
                           const Party& party) {
  Tree saved_tree_model;
  load_dt_model(saved_model_file, saved_tree_model);

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
    party.getter_phe_pub_key(phe_pub_key);

    // retrieve batch samples and encode (notice to use cur_batch_size
    // instead of default batch size to avoid unexpected batch)
    int cur_batch_size = batch_indexes.size();
    EncodedNumber* batch_phe_aggregation = new EncodedNumber[cur_batch_size];
    std::vector< std::vector<double> > batch_samples;
    for (int i = 0; i < cur_batch_size; i++) {
      batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
    }

    // step 1: organize the leaf label vector, compute the map
    LOG(INFO) << "Tree internal node num = " << saved_tree_model.internal_node_num;
    EncodedNumber *label_vector = new EncodedNumber[saved_tree_model.internal_node_num + 1];
    std::map<int, int> node_index_2_leaf_index_map;
    int leaf_cur_index = 0;
    for (int i = 0; i < pow(2, saved_tree_model.max_depth + 1) - 1; i++) {
      if (saved_tree_model.nodes[i].node_type == falcon::LEAF) {
        node_index_2_leaf_index_map.insert(std::make_pair(i, leaf_cur_index));
        label_vector[leaf_cur_index] = saved_tree_model.nodes[i].label;  // record leaf label vector
        leaf_cur_index ++;
      }
    }

    // record the ciphertext of the label
    EncodedNumber *encrypted_aggregation = new EncodedNumber[cur_batch_size];
    // for each sample
    for (int i = 0; i < cur_batch_size; i++) {
      // compute binary vector for the current sample
      std::vector<int> binary_vector = saved_tree_model.comp_predict_vector(batch_samples[i], node_index_2_leaf_index_map);
      EncodedNumber *encoded_binary_vector = new EncodedNumber[binary_vector.size()];
      EncodedNumber *updated_label_vector = new EncodedNumber[binary_vector.size()];

      // update in Robin cycle, from the last client to client 0
      if (party.party_id == party.party_num - 1) {
        // updated_label_vector = new EncodedNumber[binary_vector.size()];
        for (int j = 0; j < binary_vector.size(); j++) {
          encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
          djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j],
                            label_vector[j], encoded_binary_vector[j]);
        }
        // send to the next client
        std::string send_s;
        serialize_encoded_number_array(updated_label_vector, binary_vector.size(), send_s);
        party.send_long_message(party.party_id - 1, send_s);
      } else if (party.party_id > 0) {
        std::string recv_s;
        party.recv_long_message(party.party_id + 1, recv_s);
        deserialize_encoded_number_array(updated_label_vector, binary_vector.size(), recv_s);
        for (int j = 0; j < binary_vector.size(); j++) {
          encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
          djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j],
                            updated_label_vector[j], encoded_binary_vector[j]);
        }
        std::string resend_s;
        serialize_encoded_number_array(updated_label_vector, binary_vector.size(), resend_s);
        party.send_long_message(party.party_id - 1, resend_s);
      } else {
        // the super client update the last, and aggregate before calling share decryption
        std::string final_recv_s;
        party.recv_long_message(party.party_id + 1, final_recv_s);
        deserialize_encoded_number_array(updated_label_vector, binary_vector.size(), final_recv_s);
        for (int j = 0; j < binary_vector.size(); j++) {
          encoded_binary_vector[j].set_integer(phe_pub_key->n[0], binary_vector[j]);
          djcs_t_aux_ep_mul(phe_pub_key, updated_label_vector[j], updated_label_vector[j], encoded_binary_vector[j]);
        }
      }

      delete [] encoded_binary_vector;
      delete [] updated_label_vector;
    }

    std::string recv_s;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_s);
    deserialize_encoded_number_array(encrypted_aggregation, cur_batch_size, recv_s);

    // step 3: active party aggregates and call collaborative decryption
    EncodedNumber* decrypted_aggregation = new EncodedNumber[cur_batch_size];
    party.collaborative_decrypt(encrypted_aggregation,
                                decrypted_aggregation,
                                cur_batch_size,
                                ACTIVE_PARTY_ID);

    std::cout << "Collaboratively decryption finished" << std::endl;
    LOG(INFO) << "Collaboratively decryption finished";

    djcs_t_free_public_key(phe_pub_key);
    delete [] label_vector;
    delete [] batch_phe_aggregation;
    delete [] decrypted_aggregation;

    google::FlushLogFiles(google::INFO);
  }
}