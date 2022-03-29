//
// Created by wuyuncheng on 15/11/20.
//

#include "../../include/message/common.pb.h"
#include <falcon/utils/pb_converter/common_converter.h>

#include <glog/logging.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/message_lite.h>
#include <falcon/utils/logger/logger.h>

void serialize_int_array(std::vector<int> vec, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::IntArray int_array;
  for (int i = 0; i < vec.size(); i++) {
    int_array.add_int_item(vec[i]);
  }
  int_array.SerializeToString(&output_message);
  int_array.Clear();
}

void deserialize_int_array(std::vector<int>& vec, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::IntArray deserialized_int_array;
  if (!deserialized_int_array.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize int array message failed.";
    exit(1);
  }
  for (int i = 0; i < deserialized_int_array.int_item_size(); i++) {
    vec.push_back(deserialized_int_array.int_item(i));
  }
}

void serialize_double_array(std::vector<double> vec, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::DoubleArray double_array;
  for (int i = 0; i < vec.size(); i++) {
    double_array.add_item(vec[i]);
  }
  double_array.SerializeToString(&output_message);
}

void deserialize_double_array(std::vector<double>& vec, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::DoubleArray deserialized_double_array;
  if (!deserialized_double_array.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize double array message failed.";
    exit(1);
  }
  for (int i = 0; i < deserialized_double_array.item_size(); i++) {
    vec.push_back(deserialized_double_array.item(i));
  }
}

void serialize_double_matrix(std::vector< std::vector<double> > mat, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::DoubleMatrix double_matrix;
  int row_num = mat.size();
  int column_num = mat[0].size();
  for (int i = 0; i < row_num; i++) {
    com::nus::dbsytem::falcon::v0::DoubleArray *double_array = double_matrix.add_array();
    for (int j = 0; j < column_num; j++) {
      double_array->add_item(mat[i][j]);
    }
  }
  double_matrix.SerializeToString(&output_message);
}

void deserialize_double_matrix(std::vector< std::vector<double> >& mat, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::DoubleMatrix deserialized_double_matrix;
  if (!deserialized_double_matrix.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize double matrix message failed.";
    exit(1);
  }
  for (int i = 0; i < deserialized_double_matrix.array_size(); i++) {
    std::vector<double> row;
    const com::nus::dbsytem::falcon::v0::DoubleArray& double_array = deserialized_double_matrix.array(i);
    for (int j = 0; j < double_array.item_size(); j++) {
      row.push_back(double_array.item(j));
    }
    mat.push_back(row);
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
  n_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_n);
  value_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_value);
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
  encoded_number.Clear();
}

void deserialize_encoded_number(EncodedNumber& number, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber deserialized_encoded_number;
  if (!deserialized_encoded_number.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize fixed point encoded number message failed.";
    exit(1);
  }
  mpz_t s_n, s_value;
  mpz_init(s_n);
  mpz_init(s_value);
  mpz_set_str(s_n, deserialized_encoded_number.n().c_str(), PHE_STR_BASE);
  mpz_set_str(s_value, deserialized_encoded_number.value().c_str(), PHE_STR_BASE);

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
    n_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_n);
    value_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_value);
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
  encoded_number_array.Clear();
}

void deserialize_encoded_number_array(EncodedNumber* number_array, int size, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::EncodedNumberArray deserialized_encoded_number_array;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_message.c_str(), input_message.length());
  inputStream.SetTotalBytesLimit(PROTOBUF_SIZE_LIMIT);
  if (!deserialized_encoded_number_array.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize encoded number array message failed.";
    exit(1);
  }

  if (size != deserialized_encoded_number_array.encoded_number_size()) {
    log_info("[deserialize_encoded_number_array]: size of deserialized_encoded_number_array: " + std::to_string(deserialized_encoded_number_array.encoded_number_size()));
    log_info("[deserialize_encoded_number_array]: size: " + std::to_string(size));

    LOG(ERROR) << "Deserialized encoded number size is not expected.";
    exit(1);
  }
  // number_array = new EncodedNumber[size];
  for (int i = 0; i < size; i++) {
    const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& encoded_number = deserialized_encoded_number_array.encoded_number(i);
    mpz_t s_n, s_value;
    mpz_init(s_n);
    mpz_init(s_value);
    mpz_set_str(s_n, encoded_number.n().c_str(), PHE_STR_BASE);
    mpz_set_str(s_value, encoded_number.value().c_str(), PHE_STR_BASE);

    number_array[i].setter_n(s_n);
    number_array[i].setter_value(s_value);
    number_array[i].setter_exponent(encoded_number.exponent());
    number_array[i].setter_type(static_cast<EncodedNumberType>(encoded_number.type()));

    mpz_clear(s_n);
    mpz_clear(s_value);
  }
}

void serialize_encoded_number_matrix(EncodedNumber** number_matrix,
                                     int row_size, int column_size, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::EncodedNumberMatrix encoded_number_matrix;
  for (int i = 0; i < row_size; i++) {
    com::nus::dbsytem::falcon::v0::EncodedNumberArray *encoded_number_array = encoded_number_matrix.add_encoded_array();
    for (int j = 0; j < column_size; j++) {
      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *encoded_number = encoded_number_array->add_encoded_number();
      mpz_t g_n, g_value;
      mpz_init(g_n);
      mpz_init(g_value);
      number_matrix[i][j].getter_n(g_n);
      number_matrix[i][j].getter_value(g_value);
      char *n_str_c, *value_str_c;
      n_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_n);
      value_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_value);
      std::string n_str(n_str_c), value_str(value_str_c);

      encoded_number->set_n(n_str);
      encoded_number->set_value(value_str);
      encoded_number->set_exponent(number_matrix[i][j].getter_exponent());
      encoded_number->set_type(number_matrix[i][j].getter_type());

      mpz_clear(g_n);
      mpz_clear(g_value);
      free(n_str_c);
      free(value_str_c);
    }
  }

  encoded_number_matrix.SerializeToString(&output_message);
  encoded_number_matrix.Clear();
}


void deserialize_encoded_number_matrix(EncodedNumber** number_matrix,
                                       int row_size, int column_size, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::EncodedNumberMatrix deserialized_encoded_number_matrix;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_message.c_str(), input_message.length());
  inputStream.SetTotalBytesLimit(PROTOBUF_SIZE_LIMIT);
  if (!deserialized_encoded_number_matrix.ParseFromString(input_message)) {
    LOG(ERROR) << "Deserialize encoded number matrix message failed.";
    exit(1);
  }

  if (row_size != deserialized_encoded_number_matrix.encoded_array_size() ||
    column_size != deserialized_encoded_number_matrix.encoded_array(0).encoded_number_size()) {
    LOG(ERROR) << "Deserialized encoded number array size is not expected.";
    exit(1);
  }

  for (int i = 0; i < row_size; i++) {
    const com::nus::dbsytem::falcon::v0::EncodedNumberArray& encoded_number_array = deserialized_encoded_number_matrix.encoded_array(i);
    for (int j = 0; j < column_size; j++) {
      const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& encoded_number = encoded_number_array.encoded_number(j);
      mpz_t s_n, s_value;
      mpz_init(s_n);
      mpz_init(s_value);
      mpz_set_str(s_n, encoded_number.n().c_str(), PHE_STR_BASE);
      mpz_set_str(s_value, encoded_number.value().c_str(), PHE_STR_BASE);

      number_matrix[i][j].setter_n(s_n);
      number_matrix[i][j].setter_value(s_value);
      number_matrix[i][j].setter_exponent(encoded_number.exponent());
      number_matrix[i][j].setter_type(static_cast<EncodedNumberType>(encoded_number.type()));

      mpz_clear(s_n);
      mpz_clear(s_value);
    }
  }
}