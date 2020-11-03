//
// Created by wuyuncheng on 13/8/20.
//

#include <iostream>
#include <vector>
#include <string>

#include "network/Comm.hpp"
#include "party/party.h"

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
  Party party(party_id, party_num, network_file);
  return 0;
}