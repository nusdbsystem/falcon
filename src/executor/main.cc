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

#include <glog/logging.h>

using namespace boost;

falcon::AlgorithmName parse_algorithm_name(const std::string& name);

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  int party_id, party_num, party_type, fl_setting, use_existing_key;
  std::string network_file, log_file, data_file, key_file, algorithm_name;

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
        ("data-file", po::value<std::string>(&data_file), "file name of dataset")
        ("existing-key", po::value<int>(&use_existing_key), "whether use existing phe keys")
        ("key-file", po::value<std::string>(&key_file), "file name of phe keys")
        ("algorithm-name", po::value<std::string>(&algorithm_name), "algorithm to be run");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: options_description [options]\n";
      std::cout << description;
      return 0;
    }

    std::cout << "party-id: " << vm["party-id"].as<int>() << std::endl;
    std::cout << "party-num: " << vm["party-num"].as<int>() << std::endl;
    std::cout << "party-type: " << vm["party-type"].as<int>() << std::endl;
    std::cout << "fl-setting: " << vm["fl-setting"].as<int>() << std::endl;
    std::cout << "existing-key: " << vm["existing-key"].as<int>() << std::endl;
    std::cout << "network-file: " << vm["network-file"].as< std::string >() << std::endl;
    std::cout << "log-file: " << vm["log-file"].as< std::string >() << std::endl;
    std::cout << "data-file: " << vm["data-file"].as< std::string >() << std::endl;
    std::cout << "key-file: " << vm["key-file"].as< std::string >() << std::endl;
    std::cout << "algorithm-name: " << vm["algorithm-name"].as< std::string >() << std::endl;
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
  LOG(INFO) << "data_file: " << data_file;
  LOG(INFO) << "key_file: " << key_file;
  LOG(INFO) << "algorithm_name: " << algorithm_name;

  Party party(party_id, party_num,
      static_cast<falcon::PartyType>(party_type),
      static_cast<falcon::FLSetting>(fl_setting),
      network_file,
      data_file,
      use_existing_key,
      key_file);

  LOG(INFO) << "Parse algorithm name and run the program";
  std::cout << "Parse algorithm name and run the program" << std::endl;

  falcon::AlgorithmName name = parse_algorithm_name(algorithm_name);
  std::string algorithm_params;
  switch(name) {
    case falcon::LR:
      train_logistic_regression(party, algorithm_params);
      break;
    case falcon::DT:
      LOG(INFO) << "Decision Tree algorithm is not supported now.";
      break;
    default:
      train_logistic_regression(party, algorithm_params);
      break;
  }

  std::cout << "Finish algorithm " << std::endl;

  return 0;
}

falcon::AlgorithmName parse_algorithm_name(const std::string& name) {
  if ("logistic_regression" == name) return falcon::LR;
  if ("decision_tree" == name) return falcon::DT;
}