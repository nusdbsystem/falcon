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

#include <glog/logging.h>

using namespace boost;

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  int party_id, party_num, party_type, fl_setting, use_existing_key;
  std::string network_file, log_file, data_file, key_file;

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
        ("key-file", po::value<std::string>(&key_file), "file name of phe keys");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << "Usage: options_description [options]\n";
      std::cout << description;
      return 0;
    }

    std::cout << "vm: " << vm["party-id"].as<int>() << std::endl;
    std::cout << "vm: " << vm["party-num"].as<int>() << std::endl;
    std::cout << "vm: " << vm["party-type"].as<int>() << std::endl;
    std::cout << "vm: " << vm["fl-setting"].as<int>() << std::endl;
    std::cout << "vm: " << vm["existing-key"].as<int>() << std::endl;
    std::cout << "vm: " << vm["network-file"].as< std::string >() << std::endl;
    std::cout << "vm: " << vm["log-file"].as< std::string >() << std::endl;
    std::cout << "vm: " << vm["data-file"].as< std::string >() << std::endl;
    std::cout << "vm: " << vm["key-file"].as< std::string >() << std::endl;
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

  Party party(party_id, party_num,
      static_cast<falcon::PartyType>(party_type),
      static_cast<falcon::FLSetting>(fl_setting),
      network_file,
      data_file,
      use_existing_key,
      key_file);

//  bigint::init_thread();
//  std::vector<std::string> hosts;
//  hosts.push_back("localhost");
//  hosts.push_back("localhost");
//  hosts.push_back("localhost");
//  std::string path = "/home/wuyuncheng/Documents/falcon/third_party/MP-SPDZ/Player-Data/";
//  std::vector<ssl_socket*> sockets = setup_sockets(party_num, party_id,
//      path, hosts, 14000);
//  // Map inputs into gfp
//  int size = 10;
//  vector<float> shares(size);
//  for (int i = 0; i < size; i++) {
//    shares[i] = party_id * 0.1 + i * (-0.1) + 0.0;
//    cout << "shares[" << i << "] = " << shares[i] << endl;
//  }
//
//  cout << "Finish prepare secret shares " << endl;
//
//  // Run the computation
//  send_private_inputs<float>(shares,sockets, party_num);
//
//  // Get the result back (client_id of winning client)
//  vector<float> result = receive_result(sockets, party_num, size);
//
//  cout << "result = ";
//  for (int i = 0; i < size; i++) {
//    cout << result[i] << ",";
//  }
//  cout << endl;
//
//  for (int i = 0; i < party_num; i++)
//  {
//    cout << "delete socket " << i << endl;
//    delete sockets[i];
//  }
//
//  cout << "delete finished" << endl;

  return 0;
}