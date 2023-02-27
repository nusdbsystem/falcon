/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 13/8/20.
//

#include "falcon/utils/pb_converter/model_converter.h"

#include <iostream>

#include "../../include/message/model.pb.h"
#include <falcon/utils/logger/logger.h>
#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>

void serialize_model_publish_request(int model_id, int initiator_party_id,
                                     std::string &output_message) {
  com::nus::dbsystem::falcon::v0::ModelPublishRequest pb_request;
  pb_request.set_model_id(model_id);
  pb_request.set_initiator_party_id(initiator_party_id);

  pb_request.SerializeToString(&output_message);
  pb_request.Clear();
}

void deserialize_model_publish_request(int &model_id, int &initiator_party_id,
                                       std::string input_message) {
  com::nus::dbsystem::falcon::v0::ModelPublishRequest pb_request;
  if (!pb_request.ParseFromString(input_message)) {
    log_error("Deserialize model publish request failed.");
    exit(EXIT_FAILURE);
  }

  model_id = pb_request.model_id();
  initiator_party_id = pb_request.initiator_party_id();
}

void serialize_model_publish_response(int model_id, int initiator_party_id,
                                      int is_success, int error_code,
                                      const std::string &error_msg,
                                      std::string &output_message) {
  com::nus::dbsystem::falcon::v0::ModelPublishResponse pb_response;
  pb_response.set_model_id(model_id);
  pb_response.set_initiator_party_id(initiator_party_id);
  pb_response.set_is_success(is_success);
  pb_response.set_error_code(error_code);
  pb_response.set_error_msg(error_msg);

  pb_response.SerializeToString(&output_message);
  pb_response.Clear();
}

void deserialize_model_publish_response(int &model_id, int &initiator_party_id,
                                        int &is_success, int &error_code,
                                        std::string &error_msg,
                                        const std::string &input_message) {
  com::nus::dbsystem::falcon::v0::ModelPublishResponse pb_response;
  if (!pb_response.ParseFromString(input_message)) {
    log_error("Deserialize model publish response failed.");
    exit(EXIT_FAILURE);
  }

  model_id = pb_response.model_id();
  initiator_party_id = pb_response.initiator_party_id();
  is_success = pb_response.is_success();
  error_code = pb_response.error_code();
  error_msg = pb_response.error_msg();
}
