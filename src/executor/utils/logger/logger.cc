//
// Created by root on 10/13/21.
//

#include <falcon/utils/logger/logger.h>
#include <iostream>
#include <string>

void log_info(const std::string& str) {
  std::cout << str << std::endl;
  LOG(INFO) << str;
  google::FlushLogFiles(google::INFO);
}

void log_error(const std::string& str) {
  std::cout << str << std::endl;
  LOG(ERROR) << str;
  google::FlushLogFiles(google::INFO);
}