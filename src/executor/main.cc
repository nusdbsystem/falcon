//
// Created by wuyuncheng on 13/8/20.
//

#include <iostream>
#include <vector>
#include <string>

#include "network/Comm.hpp"
#include "party/party.h"

int main(int argc, char *argv[]) {
  int party_id, party_num;
  std::string network_file;
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
  Party party(party_id, party_num, network_file);
  return 0;
}