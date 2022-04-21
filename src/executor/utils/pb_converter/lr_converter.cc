//
// Created by wuyuncheng on 20/7/21.
//

#include <falcon/utils/pb_converter/lr_converter.h>

#include <google/protobuf/io/coded_stream.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include <google/protobuf/message_lite.h>
#include "../../include/message/lr.pb.h"

#include <iostream>

void serialize_lr_model(LinearModel lr_model, std::string & output_str) {
  com::nus::dbsytem::falcon::v0::LrModel pb_lr_model;
  pb_lr_model.set_weight_size(lr_model.weight_size);
  for (int i = 0; i < lr_model.weight_size; i++) {
    com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *encoded_number = pb_lr_model.add_model_weights();
    mpz_t g_n, g_value;
    mpz_init(g_n);
    mpz_init(g_value);
    lr_model.local_weights[i].getter_n(g_n);
    lr_model.local_weights[i].getter_value(g_value);
    char *n_str_c, *value_str_c;
    n_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_n);
    value_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_value);
    std::string n_str(n_str_c), value_str(value_str_c);

    encoded_number->set_n(n_str);
    encoded_number->set_value(value_str);
    encoded_number->set_exponent(lr_model.local_weights[i].getter_exponent());
    encoded_number->set_type(lr_model.local_weights[i].getter_type());

    mpz_clear(g_n);
    mpz_clear(g_value);
    free(n_str_c);
    free(value_str_c);
  }
  pb_lr_model.SerializeToString(&output_str);
  pb_lr_model.Clear();
}

void deserialize_lr_model(LinearModel& lr_model, std::string input_str) {
  com::nus::dbsytem::falcon::v0::LrModel deserialized_lr_model;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_str.c_str(), input_str.length());
  inputStream.SetTotalBytesLimit(PROTOBUF_SIZE_LIMIT);
  if (!deserialized_lr_model.ParseFromString(input_str)) {
    log_error("Deserialize logistic regression model message failed.");
    exit(EXIT_FAILURE);
  }

  lr_model.weight_size = deserialized_lr_model.weight_size();
  lr_model.local_weights = new EncodedNumber[lr_model.weight_size];
  // number_array = new EncodedNumber[size];
  for (int i = 0; i < lr_model.weight_size; i++) {
    const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& encoded_number = deserialized_lr_model.model_weights(i);
    mpz_t s_n, s_value;
    mpz_init(s_n);
    mpz_init(s_value);
    mpz_set_str(s_n, encoded_number.n().c_str(), PHE_STR_BASE);
    mpz_set_str(s_value, encoded_number.value().c_str(), PHE_STR_BASE);

    lr_model.local_weights[i].setter_n(s_n);
    lr_model.local_weights[i].setter_value(s_value);
    lr_model.local_weights[i].setter_exponent(encoded_number.exponent());
    lr_model.local_weights[i].setter_type(static_cast<EncodedNumberType>(encoded_number.type()));

    mpz_clear(s_n);
    mpz_clear(s_value);
  }
}
