//
// Created by wuyuncheng on 13/8/20.
//

#ifndef FALCON_SRC_EXECUTOR_PARTY_PARTY_H_
#define FALCON_SRC_EXECUTOR_PARTY_PARTY_H_

#include "../network/Comm.hpp"
#include <string>
#include <vector>

class Party {
 public:
  // current party id
  int party_id;
  // total number of parties
  int party_num;
  // communication channel with other parties
  std::vector< shared_ptr<CommParty> > channels;
  boost::asio::io_service io_service;

 public:
  Party();
  Party(int m_party_id, int m_party_num, std::string network_file);
  ~Party();
};

#endif //FALCON_SRC_EXECUTOR_PARTY_PARTY_H_
