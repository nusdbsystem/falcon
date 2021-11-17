//
// Created by wuyuncheng on 9/3/21.
//

#include <iostream>
#include <memory>
#include <string>
#include <sstream>


#include <served/served.hpp>
#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/model/model_io.h>
#include <falcon/inference/server/inference_server.h>
#include <falcon/inference/server/lr_inference_service.h>
#include <falcon/inference/server/dt_inference_service.h>
#include <falcon/inference/server/rf_inference_service.h>
#include <falcon/inference/server/gbdt_inference_service.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_ps.h>

#include <glog/logging.h>

void run_inference_server(
    const std::string& endpoint,
    const std::string& saved_model_file,
    const Party& party,
    falcon::AlgorithmName algorithm_name,
    int is_distributed,
    const std::string& distributed_network_config_pb_str) {

  served::multiplexer mux;

  // Inference endpoint
  mux.handle("/inference")
      .get([party,
            saved_model_file,
            algorithm_name,
            is_distributed,
            distributed_network_config_pb_str](served::response & res, const served::request & req) {

        // 1. receive client's requests
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

        // 2. call func to process requests in either distributed or centralized way
        int sample_size = (int) batch_indexes.size();
        auto* decrypted_labels = new EncodedNumber[sample_size];
        vector<double> labels;
        vector<vector<double>> probabilities;
        if (is_distributed == 0){
          run_active_server(party, saved_model_file, batch_indexes, algorithm_name, decrypted_labels);
        }else{
          auto ps = new LogRegParameterServer(party, distributed_network_config_pb_str);
          ps->distributed_predict(batch_indexes, decrypted_labels);
        }

        // 3. retrieve label and probabilities from model's output
        retrieve_prediction_result(sample_size, decrypted_labels, &labels, &probabilities);
        delete[] decrypted_labels;

        // 4. assemble response，return to client
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

  // served::net::server server("127.0.0.1", "8123", mux);
  served::net::server server(ip_port[0], ip_port[1], mux);
  server.run(10); // Run with a pool of 10 threads.
}

void run_active_server(const Party& party,
    const std::string& saved_model_file,
    const std::vector<int> & batch_indexes,
    falcon::AlgorithmName algorithm_name,
    EncodedNumber* decrypted_labels) {

  switch(algorithm_name) {
    case falcon::LOG_REG:
      run_active_server_lr(party, saved_model_file, batch_indexes, decrypted_labels);
      break;
//    case falcon::LINEAR_REG:
//      // TODO: add implementation
//      break;
//    case falcon::DT:
//      run_active_server_dt(endpoint, saved_model_file, party);
//      break;
//    case falcon::RF:
//      run_active_server_rf(endpoint, saved_model_file, party);
//      break;
//    case falcon::GBDT:
//      run_active_server_gbdt(endpoint, saved_model_file, party);
//      break;
    default:
      run_active_server_lr(party, saved_model_file, batch_indexes, decrypted_labels);
      break;
  }
}

void run_passive_server(const std::string& saved_model_file,
    const Party& party,
    falcon::AlgorithmName algorithm_name) {
  switch(algorithm_name) {
    case falcon::LOG_REG:
      run_passive_server_lr(saved_model_file, party);
      break;
//    case falcon::LINEAR_REG:
//      // TODO: add implementation
//      break;
    case falcon::DT:
      run_passive_server_dt(saved_model_file, party);
      break;
    case falcon::RF:
      run_passive_server_rf(saved_model_file, party);
      break;
    case falcon::GBDT:
      run_passive_server_gbdt(saved_model_file, party);
      break;
    default:
      run_passive_server_lr(saved_model_file, party);
      break;
  }
}