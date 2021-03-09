//
// Created by wuyuncheng on 15/11/20.
//

#include "../../include/message/common.pb.h"
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/message_lite.h>

void serialize_int_array(std::vector<int> vec, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::IntArray int_array;
  for (int i = 0; i < vec.size(); i++) {
    int_array.add_int_item(vec[i]);
  }
  int_array.SerializeToString(&output_message);
}

void deserialize_int_array(std::vector<int>& vec, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::IntArray deserialized_int_array;
  if (!deserialized_int_array.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize int array message failed.";
    return;
  }
  for (int i = 0; i < deserialized_int_array.int_item_size(); i++) {
    vec.push_back(deserialized_int_array.int_item(i));
  }
}

void serialize_encoded_number(EncodedNumber number, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber encoded_number;
  mpz_t g_n, g_value;
  mpz_init(g_n);
  mpz_init(g_value);
  number.getter_n(g_n);
  number.getter_value(g_value);
  char *n_str_c, *value_str_c;
  n_str_c = mpz_get_str(NULL, 10, g_n);
  value_str_c = mpz_get_str(NULL, 10, g_value);
  std::string n_str(n_str_c), value_str(value_str_c);
  encoded_number.set_n(n_str);
  encoded_number.set_value(value_str);
  encoded_number.set_exponent(number.getter_exponent());
  encoded_number.set_type(number.getter_type());

  encoded_number.SerializeToString(&output_message);

  mpz_clear(g_n);
  mpz_clear(g_value);
  free(n_str_c);
  free(value_str_c);
}

void deserialize_encoded_number(EncodedNumber& number, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber deserialized_encoded_number;
  if (!deserialized_encoded_number.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize fixed point encoded number message failed.";
    return;
  }
  mpz_t s_n, s_value;
  mpz_init(s_n);
  mpz_init(s_value);
  mpz_set_str(s_n, deserialized_encoded_number.n().c_str(), 10);
  mpz_set_str(s_value, deserialized_encoded_number.value().c_str(), 10);

  number.setter_n(s_n);
  number.setter_value(s_value);
  number.setter_exponent(deserialized_encoded_number.exponent());
  number.setter_type(static_cast<EncodedNumberType>(deserialized_encoded_number.type()));

  mpz_clear(s_n);
  mpz_clear(s_value);
}

void serialize_encoded_number_array(EncodedNumber* number_array, int size, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::EncodedNumberArray encoded_number_array;
  for (int i = 0; i < size; i++) {
    com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *encoded_number = encoded_number_array.add_encoded_number();
    mpz_t g_n, g_value;
    mpz_init(g_n);
    mpz_init(g_value);
    number_array[i].getter_n(g_n);
    number_array[i].getter_value(g_value);
    char *n_str_c, *value_str_c;
    n_str_c = mpz_get_str(NULL, 10, g_n);
    value_str_c = mpz_get_str(NULL, 10, g_value);
    std::string n_str(n_str_c), value_str(value_str_c);

    encoded_number->set_n(n_str);
    encoded_number->set_value(value_str);
    encoded_number->set_exponent(number_array[i].getter_exponent());
    encoded_number->set_type(number_array[i].getter_type());

    mpz_clear(g_n);
    mpz_clear(g_value);
    free(n_str_c);
    free(value_str_c);
  }
  encoded_number_array.SerializeToString(&output_message);
}

void deserialize_encoded_number_array(EncodedNumber* number_array, int size, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::EncodedNumberArray deserialized_encoded_number_array;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_message.c_str(), input_message.length());
  inputStream.SetTotalBytesLimit(PROTOBUF_SIZE_LIMIT, PROTOBUF_SIZE_LIMIT);
  if (!deserialized_encoded_number_array.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize encoded number array message failed.";
    return;
  }

  if (size != deserialized_encoded_number_array.encoded_number_size()) {
    LOG(ERROR) << "Deserialized encoded number size is not expected.";
    return;
  }
  // number_array = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& encoded_number = deserialized_encoded_number_array.encoded_number(i);
    mpz_t s_n, s_value;
    mpz_init(s_n);
    mpz_init(s_value);
    mpz_set_str(s_n, encoded_number.n().c_str(), 10);
    mpz_set_str(s_value, encoded_number.value().c_str(), 10);

    number_array[i].setter_n(s_n);
    number_array[i].setter_value(s_value);
    number_array[i].setter_exponent(encoded_number.exponent());
    number_array[i].setter_type(static_cast<EncodedNumberType>(encoded_number.type()));

    mpz_clear(s_n);
    mpz_clear(s_value);
  }
}