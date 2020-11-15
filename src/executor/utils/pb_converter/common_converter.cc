//
// Created by wuyuncheng on 15/11/20.
//

#include "../../include/message/common.pb.h"
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>

void serialize_int_array(std::vector<int> vec, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::IntArray int_array;
  for (int i = 0; i < vec.size(); i++) {
    int_array.add_int_item(vec[i]);
  }
  int_array.SerializeToString(&output_message);
}

void deserialize_int_array(std::vector<int>& vec, std::string input_message) {
  com::nus::dbsytem::falcon::v0::IntArray deserialized_int_array;
  if (!deserialized_int_array.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize int array message failed.";
    return;
  }
  for (int i = 0; i < deserialized_int_array.int_item_size(); i++) {
    vec.push_back(deserialized_int_array.int_item(i));
  }
}