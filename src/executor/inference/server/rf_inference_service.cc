//
// Created by wuyuncheng on 9/6/21.
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
#include <falcon/utils/math/math_ops.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using com::nus::dbsytem::falcon::v0::inference::PredictionRequest;
using com::nus::dbsytem::falcon::v0::inference::PredictionResponse;
using com::nus::dbsytem::falcon::v0::inference::InferenceService;

// Logic and data behind the server's behavior.
class RFInferenceServiceImpl final : public InferenceService::Service {
 public:
  explicit RFInferenceServiceImpl(
      const std::string& saved_model_file,
      const Party& party) {
    party_ = party;
    load_rf_model(saved_model_file, saved_rf_model_, n_estimator_);
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

    // retrieve batch samples and encode (notice to use cur_batch_size
    // instead of default batch size to avoid unexpected batch)
    std::vector< std::vector<double> > batch_samples;
    for (int i = 0; i < sample_num; i++) {
      batch_samples.push_back(party_.getter_local_data()[batch_indexes[i]]);
    }

    // init predicted forest labels to record the predictions, the first dimension
    // is the number of trees in the forest, and the second dimension is the number
    // of the samples in the predicted dataset
    EncodedNumber** predicted_forest_labels = new EncodedNumber*[n_estimator_];
    EncodedNumber** decrypted_predicted_forest_labels = new EncodedNumber*[n_estimator_];
    for (int tree_id = 0; tree_id < n_estimator_; tree_id++) {
      predicted_forest_labels[tree_id] = new EncodedNumber[sample_num];
      decrypted_predicted_forest_labels[tree_id] = new EncodedNumber[sample_num];
    }

    // compute predictions
    for (int tree_id = 0; tree_id < n_estimator_; tree_id++) {
      // compute predictions for each tree in the forest
      saved_rf_model_[tree_id].predict(party_,
                                          batch_samples,
                                          sample_num,
                                          predicted_forest_labels[tree_id]);
      // decrypt the predicted labels and compute the accuracy
      party_.collaborative_decrypt(predicted_forest_labels[tree_id],
                                  decrypted_predicted_forest_labels[tree_id],
                                  sample_num, ACTIVE_PARTY_ID);
    }

    // decode decrypted predicted labels
    std::vector< std::vector<double> > decoded_predicted_forest_labels (
        n_estimator_, std::vector<double>(sample_num));
    for (int tree_id = 0; tree_id < n_estimator_; tree_id++) {
      for (int i = 0; i < sample_num; i++) {
        decrypted_predicted_forest_labels[tree_id][i].decode(decoded_predicted_forest_labels[tree_id][i]);
      }
    }

    // calculate accuracy by the super client
    std::vector<double> labels;
    std::vector< std::vector<double> > probabilities;
    if (party_.party_type == falcon::ACTIVE_PARTY) {
      for (int i = 0; i < sample_num; i++) {
        std::vector<double> forest_labels_i;
        for (int tree_id = 0; tree_id < n_estimator_; tree_id++) {
          forest_labels_i.push_back(decoded_predicted_forest_labels[tree_id][i]);
        }
        // compute mode or average, depending on the tree type
        std::vector<double> prob;
        if (saved_rf_model_[0].type == falcon::CLASSIFICATION) {
          double pred = mode(forest_labels_i);
          labels.push_back(pred);
          for (int k = 0; k < saved_rf_model_[0].class_num; k++) {
            if (pred == k) {
              prob.push_back(CERTAIN_PROBABILITY);
            } else {
              prob.push_back(ZERO_PROBABILITY);
            }
          }
          probabilities.push_back(prob);
        } else {
          labels.push_back(average(forest_labels_i));
          prob.push_back(CERTAIN_PROBABILITY);
        }
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

    // free memory
    for (int tree_id = 0; tree_id < n_estimator_; tree_id++) {
      delete [] predicted_forest_labels[tree_id];
      delete [] decrypted_predicted_forest_labels[tree_id];
    }
    delete [] predicted_forest_labels;
    delete [] decrypted_predicted_forest_labels;

    google::FlushLogFiles(google::INFO);
    return Status::OK;
  }

 private:
  Party party_;
  std::vector<Tree> saved_rf_model_;
  int n_estimator_;
};


void run_active_server_rf(const std::string& endpoint,
    const std::string& saved_model_file,
    const Party& party) {
  RFInferenceServiceImpl service(saved_model_file, party);

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

void run_passive_server_rf(const std::string& saved_model_file,
    const Party& party) {
  std::vector<Tree> saved_rf_model;
  int n_estimator;
  load_rf_model(saved_model_file, saved_rf_model, n_estimator);

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
    std::vector< std::vector<double> > batch_samples;
    for (int i = 0; i < cur_batch_size; i++) {
      batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
    }

    // init predicted forest labels to record the predictions, the first dimension
    // is the number of trees in the forest, and the second dimension is the number
    // of the samples in the predicted dataset
    EncodedNumber** predicted_forest_labels = new EncodedNumber*[n_estimator];
    EncodedNumber** decrypted_predicted_forest_labels = new EncodedNumber*[n_estimator];
    for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
      predicted_forest_labels[tree_id] = new EncodedNumber[cur_batch_size];
      decrypted_predicted_forest_labels[tree_id] = new EncodedNumber[cur_batch_size];
    }

    // compute predictions
    for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
      // compute predictions for each tree in the forest
      saved_rf_model[tree_id].predict(const_cast<Party &>(party),
          batch_samples,
          cur_batch_size,
          predicted_forest_labels[tree_id]);
      // decrypt the predicted labels and compute the accuracy
      party.collaborative_decrypt(predicted_forest_labels[tree_id],
          decrypted_predicted_forest_labels[tree_id],
          cur_batch_size, ACTIVE_PARTY_ID);
    }

//    // decode decrypted predicted labels
//    std::vector< std::vector<double> > decoded_predicted_forest_labels (
//        n_estimator, std::vector<double>(cur_batch_size));
//    for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
//      for (int i = 0; i < cur_batch_size; i++) {
//        decrypted_predicted_forest_labels[tree_id][i].decode(decoded_predicted_forest_labels[tree_id][i]);
//      }
//    }

    std::cout << "Collaboratively decryption finished" << std::endl;
    LOG(INFO) << "Collaboratively decryption finished";

    // free memory
    for (int tree_id = 0; tree_id < n_estimator; tree_id++) {
      delete [] predicted_forest_labels[tree_id];
      delete [] decrypted_predicted_forest_labels[tree_id];
    }
    delete [] predicted_forest_labels;
    delete [] decrypted_predicted_forest_labels;

    google::FlushLogFiles(google::INFO);
  }
}