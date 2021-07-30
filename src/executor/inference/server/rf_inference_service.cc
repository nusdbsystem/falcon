//
// Created by wuyuncheng on 9/6/21.
//

#include <iostream>
#include <memory>
#include <string>

#include <served/served.hpp>

#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/model/model_io.h>
#include <falcon/inference/server/dt_inference_service.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/io_util.h>

#include <glog/logging.h>
#include <falcon/utils/math/math_ops.h>


void run_active_server_rf(const std::string& endpoint,
                          const std::string& saved_model_file,
                          const Party& party) {
  // The actual processing. Add process logic here
  ForestModel saved_forest_model_;
  std::string saved_model_string;
  load_pb_model_string(saved_model_string, saved_model_file);
  deserialize_random_forest_model(saved_forest_model_, saved_model_string);

  served::multiplexer mux;

  mux.handle("/inference")
      .get([&](served::response & res, const served::request & req) {
        std::cout << "Receive client's request" << std::endl;
        LOG(INFO) << "Receive client's request";
        int sample_num = 0;
        std::vector<int> batch_indexes;
        // iterate all query param, sparse the client request
        for ( const auto & query_param : req.query )
        {
          if (query_param.first == "sampleNum") {
            sample_num = std::stoi(query_param.second);
          }
          if (query_param.first == "sampleIds") {
            stringstream ss(query_param.second);
            string id;
            while (getline(ss, id, ',')) {
              batch_indexes.push_back(std::stoi(id));
            }
          }
        }
        std::cout << "Parse client's request finished" << std::endl;
        LOG(INFO) << "Parse client's request finished";

        // send batch indexes to passive parties
        std::string batch_indexes_str;
        serialize_int_array(batch_indexes, batch_indexes_str);
        for (int i = 0; i < party.party_num; i++) {
          if (i != party.party_id) {
            party.send_long_message(i, batch_indexes_str);
          }
        }
        std::cout << "Broadcast client's batch requests to other parties" << std::endl;
        LOG(INFO) << "Broadcast client's batch requests to other parties";

        // retrieve batch samples and encode (notice to use cur_batch_size
        // instead of default batch size to avoid unexpected batch)
        EncodedNumber* predicted_labels = new EncodedNumber[sample_num];
        std::vector< std::vector<double> > batch_samples;
        for (int i = 0; i < sample_num; i++) {
          batch_samples.push_back(party.getter_local_data()[batch_indexes[i]]);
        }
        saved_forest_model_.predict(const_cast<Party &>(party), batch_samples, sample_num, predicted_labels);

        // step 3: active party aggregates and call collaborative decryption
        EncodedNumber* decrypted_labels = new EncodedNumber[sample_num];
        party.collaborative_decrypt(predicted_labels,
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
          std::vector<double> prob;
          if (saved_forest_model_.forest_trees[0].type == falcon::CLASSIFICATION) {
            // TODO: record the detailed probability, here assume 1.0
            for (int k = 0; k < saved_forest_model_.forest_trees[0].class_num; k++) {
              if (t == k) {
                prob.push_back(CERTAIN_PROBABILITY);
              } else {
                prob.push_back(ZERO_PROBABILITY);
              }
            }
            probabilities.push_back(prob);
          } else {
            prob.push_back(CERTAIN_PROBABILITY);
            probabilities.push_back(prob);
          }
        }
        std::cout << "Compute prediction finished" << std::endl;
        LOG(INFO) << "Compute prediction finished";
        delete [] predicted_labels;
        delete [] decrypted_labels;
        google::FlushLogFiles(google::INFO);

        // assemble response
        res << "The batch prediction results are as follows.\n";
        res << "Predicted sample num: " << NumberToString(sample_num) << "\n";
        for (int i = 0; i < sample_num; i++) {
          res << "\t sample " << NumberToString(i) << "'s label: " << NumberToString(labels[i]) << ". ";
          res << "probabilities: [";
          for (int j = 0; j < probabilities[0].size(); j++) {
            if (j != probabilities[0].size() - 1) {
              res << NumberToString(probabilities[i][j]) << ", ";
            } else {
              res << NumberToString(probabilities[i][j]) << "]\n";
            }
          }
        }
      });

  std::cout << "Try this example with:" << std::endl;
  std::cout << " curl http://localhost:50051/inference" << std::endl;

  // convert endpoint to ip and port
  std::vector<std::string> ip_port;
  stringstream ss(endpoint);
  string str;
  while (getline(ss, str, ',')) {
    ip_port.push_back(str);
  }

  served::net::server server(ip_port[0], ip_port[1], mux);
  server.run(10); // Run with a pool of 10 threads.
}

void run_passive_server_rf(const std::string& saved_model_file,
                           const Party& party) {
  ForestModel saved_forest_model;
  std::string saved_model_string;
  load_pb_model_string(saved_model_string, saved_model_file);
  deserialize_random_forest_model(saved_forest_model, saved_model_string);

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
    saved_forest_model.predict(const_cast<Party &>(party), batch_samples, cur_batch_size, predicted_labels);

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