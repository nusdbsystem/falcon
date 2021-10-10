//
// Created by wuyuncheng on 13/8/20.
//

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <boost/program_options.hpp>

#include "falcon/party/party.h"
#include "falcon/algorithm/vertical/linear_model/logistic_regression_builder.h"
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/algorithm/vertical/tree/forest_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include "falcon/inference/server/inference_server.h"
#include "falcon/distributed/worker.h"
#include "falcon/algorithm/vertical/linear_model/logistic_regression_ps.h"
#include "falcon/utils/base64.h"
#include <chrono>

#include <glog/logging.h>

using namespace boost;
using namespace std::chrono;

falcon::AlgorithmName parse_algorithm_name(const std::string& name);

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  int party_id, party_num, party_type, fl_setting, use_existing_key;
  std::string network_file, log_file, data_input_file, data_output_file, key_file, model_save_file, model_report_file;
  std::string algorithm_name, algorithm_params;

  // for distributed training
  int is_distributed = 0;
  std::string distributed_network_file;
  int worker_id;
  // parameter server:0, worker:1
  int distributed_role = 0;

  // for serving params
  int is_inference = 0;
  std::string inference_endpoint;

  try {
    namespace po = boost::program_options;
    po::options_description description("Usage:");
    description.add_options()
        ("help,h", "display this help message")
        ("version,v", "display the version number")
        ("party-id", po::value<int>(&party_id), "current party id")
        ("party-num", po::value<int>(&party_num), "total party num")
        ("party-type", po::value<int>(&party_type), "type of this party, active or passive")
        ("fl-setting", po::value<int>(&fl_setting), "federated learning setting, horizontal or vertical")
        ("network-file", po::value<std::string>(&network_file), "file name of network configurations")
        ("log-file", po::value<std::string>(&log_file), "file name of log destination")
        ("data-input-file", po::value<std::string>(&data_input_file), "input file name of dataset")
        ("data-output-file", po::value<std::string>(&data_output_file), "output file name of dataset")
        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run")
        // algorithm-params is not needed in inference stage
        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
        // model-report-file is not needed in inference stage
        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request")
        ("is-distributed", po::value<int>(&is_distributed), "is distributed")
        ("distributed-train-network-file", po::value<string>(&distributed_network_file), "ps network file")
        ("worker-id", po::value<int>(&worker_id), "worker id")
        ("distributed-role", po::value<int>(&distributed_role), "distributed role, worker:1, parameter server:0");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: options_description [options]\n";
      std::cout << description;
      return 0;
    }

    std::cout << "parse parameters correct" << std::endl;

    std::cout << "party-id: " << vm["party-id"].as<int>() << std::endl;
    std::cout << "party-num: " << vm["party-num"].as<int>() << std::endl;
    std::cout << "party-type: " << vm["party-type"].as<int>() << std::endl;
    std::cout << "fl-setting: " << vm["fl-setting"].as<int>() << std::endl;
    std::cout << "existing-key: " << vm["existing-key"].as<int>() << std::endl;
    std::cout << "network-file: " << vm["network-file"].as< std::string >() << std::endl;
    std::cout << "log-file: " << vm["log-file"].as< std::string >() << std::endl;
    std::cout << "data-input-file: " << vm["data-input-file"].as< std::string >() << std::endl;
    std::cout << "data-output-file: " << vm["data-output-file"].as< std::string >() << std::endl;
    std::cout << "key-file: " << vm["key-file"].as< std::string >() << std::endl;
    std::cout << "algorithm-name: " << vm["algorithm-name"].as< std::string >() << std::endl;
    std::cout << "algorithm-params: " << vm["algorithm-params"].as< std::string >() << std::endl;
    std::cout << "model-save-file: " << vm["model-save-file"].as< std::string >() << std::endl;
    std::cout << "model-report-file: " << vm["model-report-file"].as< std::string >() << std::endl;
    std::cout << "is-inference: " << vm["is-inference"].as<int>() << std::endl;
    std::cout << "inference-endpoint: " << vm["inference-endpoint"].as< std::string >() << std::endl;
    std::cout << "is-distributed: " << vm["is-distributed"].as< int >() << std::endl;
    std::cout << "distributed-train-network-file: " << vm["distributed-train-network-file"].as< std::string >() << std::endl;
    std::cout << "worker-id: " << vm["worker-id"].as< int >() << std::endl;
    std::cout << "distributed-role: " << vm["distributed-role"].as< int >() << std::endl;
  }
  catch(std::exception& e)
  {
    cout << e.what() << "\n";
    return 1;
  }

  FLAGS_log_dir = log_file;
  LOG(INFO) << "Init log file.";

  LOG(INFO) << "party_id: " << party_id;
  LOG(INFO) << "party_num: " << party_num;
  LOG(INFO) << "party_type: " << party_type;
  LOG(INFO) << "fl_setting: " << fl_setting;
  LOG(INFO) << "use_existing_key: " << use_existing_key;
  LOG(INFO) << "network_file: " << network_file;
  LOG(INFO) << "log_file: " << log_file;
  LOG(INFO) << "data_input_file: " << data_input_file;
  LOG(INFO) << "data_output_file: " << data_output_file;
  LOG(INFO) << "key_file: " << key_file;
  LOG(INFO) << "algorithm_name: " << algorithm_name;
  LOG(INFO) << "algorithm_params: " << algorithm_params;
  LOG(INFO) << "model_save_file: " << model_save_file;
  LOG(INFO) << "model_report_file: " << model_report_file;

  LOG(INFO) << "is_distributed: " << is_distributed;
  LOG(INFO) << "distributed_network_file: " << distributed_network_file;
  LOG(INFO) << "worker_id: " << worker_id;
  LOG(INFO) << "distributed_role: " << distributed_role;

  // TODO: should be read from the coordinator
  is_inference = 0;
  inference_endpoint = DEFAULT_INFERENCE_ENDPOINT;

  LOG(INFO) << "is_inference: " << is_inference;
  LOG(INFO) << "inference_endpoint: " << inference_endpoint;

  try {
    auto start_time = high_resolution_clock::now();

    Party party(party_id, party_num,
                static_cast<falcon::PartyType>(party_type),
                static_cast<falcon::FLSetting>(fl_setting),
                data_input_file);

    LOG(INFO) << "Parse algorithm name and run the program";
    std::cout << "Parse algorithm name and run the program" << std::endl;

    falcon::AlgorithmName parsed_algorithm_name = parse_algorithm_name(algorithm_name);

    // decode the base64 string to pb string, assume that only accept config string from coordinator
    // TODO: check if reading network file from disk is allowed
    std::string algorithm_params_pb_str = base64_decode_to_pb_string(algorithm_params);
    std::string network_config_pb_str = base64_decode_to_pb_string(network_file);
    std::string ps_network_config_pb_str = base64_decode_to_pb_string(distributed_network_file);

    // if non-distributed train or inference
    if (is_distributed == 0){
      // for non-distributed train or inference, init the network and keys directly
      party.init_network_channels(network_config_pb_str);
      party.init_phe_keys(use_existing_key, key_file);
      if (is_inference) {
        LOG(INFO) << "Execute inference logic\n";
        // invoke creating endpoint for inference requests
        if (party_type == falcon::ACTIVE_PARTY) {
          run_inference_server(inference_endpoint,
                               model_save_file,
                               party,
                               parsed_algorithm_name,
                               is_distributed,
                               ps_network_config_pb_str);
        } else {
          run_passive_server(model_save_file, party, parsed_algorithm_name);
        }
      } else {
        LOG(INFO) << "Execute training logic\n";
        switch(parsed_algorithm_name) {
          case falcon::LR:
            train_logistic_regression(&party, algorithm_params_pb_str, model_save_file, model_report_file);
            break;
          case falcon::DT:
            train_decision_tree(party, algorithm_params_pb_str, model_save_file, model_report_file);
            break;
          case falcon::RF:
            train_random_forest(party, algorithm_params_pb_str, model_save_file, model_report_file);
            break;
          case falcon::GBDT:
            train_gbdt(party, algorithm_params, model_save_file, model_report_file);
            break;
          default:
            train_logistic_regression(&party, algorithm_params_pb_str, model_save_file, model_report_file);
            break;
        }
        std::cout << "Finish algorithm " << std::endl;
      }
    }

    // if distributed train or inference, and distributed_role = parameter server
    if (is_distributed == 1 && distributed_role == 0){
      LOG(INFO) << "Execute as parameter server\n";
      if (is_inference) {
        LOG(INFO) << "Execute distributed inference logic\n";
        // if parameter server, for inference, does not init channels with passive ps
        // party.init_network_channels(network_config_pb_str);
        party.init_phe_keys(use_existing_key, key_file);
        if (party_type == falcon::ACTIVE_PARTY) {
          // only active party have parameter server, which is a mapper
          run_inference_server(inference_endpoint,
                               model_save_file,
                               party,
                               parsed_algorithm_name,
                               is_distributed,
                               ps_network_config_pb_str);
        } else{
          LOG(INFO) << "Passive Party doesn't run parameter server in distributed inference\n";
        }
      } else{
        LOG(INFO) << "Execute distributed training logic\n";
        // if parameter server, init the party itself network channels and phe keys
        party.init_network_channels(network_config_pb_str);
        party.init_phe_keys(use_existing_key, key_file);
        switch(parsed_algorithm_name) {
          case falcon::LR:
            launch_lr_parameter_server(&party,
                                       algorithm_params_pb_str,
                                       ps_network_config_pb_str,
                                       model_save_file,
                                       model_report_file);
            break;
          case falcon::DT:
            LOG(ERROR) << "Type " << "falcon::DT" << " not implemented";
            exit(1);
          case falcon::RF:
            LOG(ERROR) << "Type " << "falcon::RF" << " not implemented";
            exit(1);
          default:
            launch_lr_parameter_server(&party, algorithm_params_pb_str,ps_network_config_pb_str,
                                       model_save_file, model_report_file);
            break;
        }
      }
      LOG(INFO) << "Parameter Server are running\n";
    }

    // if distributed train or inference, and distributed_role = worker
    if (is_distributed == 1 && distributed_role == 1){
      // if distributed workers, need to init the network channels
      // but workers do not init phe keys but will receive it from ps
      party.init_network_channels(network_config_pb_str);
      // party.init_phe_keys(use_existing_key, key_file);

      // worker is created to communicate with parameter server
      auto worker = new Worker(ps_network_config_pb_str, worker_id);

      if (is_inference) {
        LOG(INFO) << "Execute distributed inference logic\n";
        if (party_type == falcon::ACTIVE_PARTY) {
          // receive batch index from ps
          while (true) {
            std::vector<double> labels;
            std::vector< std::vector<double> > probabilities;
            // receive mini batch index from parameter server
            std::string mini_batch_indexes_str;
            worker->recv_long_message_from_ps(mini_batch_indexes_str);
            std::vector<int> mini_batch_indexes;
            deserialize_int_array(mini_batch_indexes, mini_batch_indexes_str);
            auto* decrypted_labels = new EncodedNumber[mini_batch_indexes.size()];
            // do prediction
            run_active_server(party, model_save_file, mini_batch_indexes, parsed_algorithm_name, decrypted_labels);
            // send result to parameter
            std::string decrypted_predict_label_str;
            serialize_encoded_number_array(decrypted_labels, mini_batch_indexes.size(), decrypted_predict_label_str);
            worker->send_long_message_to_ps(decrypted_predict_label_str);
            delete [] decrypted_labels;
          }
        } else {
          run_passive_server(model_save_file, party, parsed_algorithm_name);
        }
      }else{
        LOG(INFO) << "Execute distributed training logic\n";
        if (distributed_role==1){
          LOG(INFO) << "Execute as worker\n";
          switch(parsed_algorithm_name) {
            case falcon::LR:
              train_logistic_regression(&party,
                                        algorithm_params_pb_str,
                                        model_save_file,
                                        model_report_file,
                                        is_distributed,
                                        worker);
              break;
            case falcon::DT:
              LOG(ERROR) << "Type " << "falcon::DT" << " not implemented";
              exit(1);
            case falcon::RF:
              LOG(ERROR) << "Type " << "falcon::RF" << " not implemented";
              exit(1);
            case falcon::GBDT:
              LOG(ERROR) << "Type " << "falcon::RF" << " not implemented";
              exit(1);
            default:
              train_logistic_regression(&party,
                                        algorithm_params_pb_str,
                                        model_save_file,
                                        model_report_file,
                                        is_distributed,
                                        worker);
              break;
          }
          std::cout << "Finish distributed training algorithm " << std::endl;
        }
      }
      delete worker;
    }

    // After function call
    auto stop_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop_time - start_time);
    cout << "Time taken by main func: "
    << duration.count() << " microseconds" << endl;
    LOG(INFO) << "Time taken by main func: "
    << duration.count() << " microseconds" << model_report_file;

    return EXIT_SUCCESS;
  }
  catch(std::exception& e)
  {
    cout << e.what() << "\n";
    return 1;
  }
}

falcon::AlgorithmName parse_algorithm_name(const std::string& name) {
  falcon::AlgorithmName output = falcon::LR;
  if ("logistic_regression" == name) output = falcon::LR;
  if ("decision_tree" == name) output = falcon::DT;
  if ("random_forest" == name) output = falcon::RF;
  if ("gbdt" == name) output = falcon::GBDT;
  return output;
}
