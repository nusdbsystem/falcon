//
// Created by wuyuncheng on 13/8/20.
//

#include <iostream>
#include <vector>
#include <string>

#include "falcon/network/Comm.hpp"
#include "falcon/party/party.h"
#include "falcon/operator/mpc/spdz_connector.h"

#include <glog/logging.h>

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  int party_id, party_num;
  std::string network_file, log_dir;
  if (argv[1] != NULL) {
    party_id = atoi(argv[1]);
  }
  if (argc > 2) {
    if (argv[2] != NULL) {
      party_num = atoi(argv[2]);
    }
  }
  if (argc > 3) {
    if (argv[3] != NULL) {
      network_file = argv[3];
    }
  }
  if (argc > 4) {
    if (argv[4] != NULL) {
      log_dir = argv[4];
    }
  }
  FLAGS_log_dir = log_dir;
  LOG(INFO) << "Init glog file.";
  //Party party(party_id, party_num, network_file);

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