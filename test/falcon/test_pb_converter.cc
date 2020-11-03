//
// Created by wuyuncheng on 13/8/20.
//

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#include "../../src/utils/pb_converter/model_pb_converter.h"

using namespace std;

TEST(PB_Converter, ModelPublishRequest) {
  int model_id = 1;
  int initiator_party_id = 1001;
  std::string output_message;
  serialize_model_publish_request(model_id,
      initiator_party_id,
      output_message);

  int deserialized_model_id;
  int deserialized_initiator_party_id;
  deserialize_model_publish_request(deserialized_model_id,
      deserialized_initiator_party_id,
      output_message);

  EXPECT_EQ(1, deserialized_model_id);
  EXPECT_EQ(1001, deserialized_initiator_party_id);
}

TEST(PB_Converter, ModelPublishResponse) {
  int model_id = 1;
  int initiator_party_id = 1001;
  int is_success = 0;
  int error_code = 2001;
  std::string error_msg = "Model id does not exist.";
  std::string output_message;
  serialize_model_publish_response(model_id,
      initiator_party_id,
      is_success,
      error_code,
      error_msg,
      output_message);

  int deserialized_model_id;
  int deserialized_initiator_party_id;
  int deserialized_is_success;
  int deserialized_error_code;
  std::string deserialized_error_msg;
  deserialize_model_publish_response(deserialized_model_id,
      deserialized_initiator_party_id,
      deserialized_is_success,
      deserialized_error_code,
      deserialized_error_msg,
      output_message);

  EXPECT_EQ(1, deserialized_model_id);
  EXPECT_EQ(1001, deserialized_initiator_party_id);
  EXPECT_EQ(0, deserialized_is_success);
  EXPECT_EQ(2001, deserialized_error_code);
  EXPECT_TRUE(error_msg == deserialized_error_msg);
}