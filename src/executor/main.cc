//
// Created by wuyuncheng on 13/8/20.
//

#include <iostream>
#include <vector>
#include <string>
#include <boost/program_options.hpp>

#include "falcon/network/Comm.hpp"
#include "falcon/party/party.h"
#include "falcon/operator/mpc/spdz_connector.h"
#include "falcon/algorithm/vertical/linear_model/logistic_regression.h"
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include "falcon/inference/server/inference_server.h"

#include <glog/logging.h>

using namespace boost;

falcon::AlgorithmName parse_algorithm_name(const std::string& name);

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  int party_id, party_num, party_type, fl_setting, use_existing_key;
  std::string network_file, log_file, data_input_file, data_output_file, key_file, model_save_file, model_report_file;
  std::string algorithm_name, algorithm_params;

  // add for serving params
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
        ("algorithm-params", po::value<std::string>(&algorithm_params), "parameters for the algorithm")
        ("model-save-file", po::value<std::string>(&model_save_file), "model save file name")
        ("model-report-file", po::value<std::string>(&model_report_file), "model report file name")
        ("is-inference", po::value<int>(&is_inference), "whether it is an inference job")
        ("inference-endpoint", po::value<std::string>(&inference_endpoint), "endpoint to listen inference request");

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
    //std::cout << "is-inference: " << vm["is-inference"].as<int>() << std::endl;
    //std::cout << "inference-endpoint: " << vm["inference-endpoint"].as< std::string >() << std::endl;
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

  //is_inference = 1;
  inference_endpoint = DEFAULT_INFERENCE_ENDPOINT;

  LOG(INFO) << "is_inference: " << is_inference;
  LOG(INFO) << "inference_endpoint: " << inference_endpoint;

  Party party(party_id, party_num,
      static_cast<falcon::PartyType>(party_type),
      static_cast<falcon::FLSetting>(fl_setting),
      network_file,
      data_input_file,
      use_existing_key,
      key_file);

  LOG(INFO) << "Parse algorithm name and run the program";
  std::cout << "Parse algorithm name and run the program" << std::endl;

  falcon::AlgorithmName name = parse_algorithm_name(algorithm_name);

#if IS_INFERENCE == 0
  switch(name) {
      case falcon::LR:
        train_logistic_regression(party, algorithm_params, model_save_file, model_report_file);
        break;
      case falcon::DT:
        train_decision_tree(party, algorithm_params, model_save_file, model_report_file);
        break;
      default:
        train_logistic_regression(party, algorithm_params, model_save_file, model_report_file);
        break;
    }
  std::cout << "Finish algorithm " << std::endl;
#else
  // invoke creating endpoint for inference requests
  // TODO: there is a problem when using grpc server with spdz_logistic_function_computation
  // TODO: now alleviate the problem by not including during training (need to check later)
  if (party_type == falcon::ACTIVE_PARTY) {
    RunActiveServerLR(inference_endpoint, model_save_file, party);
  } else {
    RunPassiveServerLR(model_save_file, party);
  }
#endif
  return 0;
}

falcon::AlgorithmName parse_algorithm_name(const std::string& name) {
  if ("logistic_regression" == name) return falcon::LR;
  if ("decision_tree" == name) return falcon::DT;
}