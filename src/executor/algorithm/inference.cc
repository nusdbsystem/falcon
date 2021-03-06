//
// Created by wuyuncheng on 6/3/21.
//

#include "falcon/algorithm/inference.h"

Inference::Inference(const std::string& endpoint) {
  size_t colon_pos = endpoint.find(':');
  endpoint_ip = endpoint.substr(0, colon_pos);
  endpoint_port = std::stoi(endpoint.substr(colon_pos + 1));
}