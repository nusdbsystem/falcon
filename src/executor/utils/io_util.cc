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
// Created by wuyuncheng on 12/11/20.
//

#include <falcon/utils/base64.h>
#include <falcon/utils/io_util.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

std::vector<std::vector<double>> read_dataset(const std::string &data_file,
                                              char delimiter) {
  std::ifstream data_infile(data_file);
  if (!data_infile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::vector<std::vector<double>> data;
  std::string line;
  while (std::getline(data_infile, line)) {
    std::vector<double> items;
    std::istringstream ss(line);
    std::string item;
    // split line with delimiter, default ','
    while (getline(ss, item, delimiter)) {
      items.push_back(::atof(item.c_str()));
    }
    data.push_back(items);
  }
  data_infile.close();

  return data;
}

void write_dataset_to_file(std::vector<std::vector<double>> data,
                           char delimiter, const std::string &data_file) {
  std::ofstream write_outfile(data_file);
  if (!write_outfile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  int row_num = data.size();
  int column_num = data[0].size();
  for (int i = 0; i < row_num; i++) {
    std::string line;
    for (int j = 0; j < column_num; j++) {
      // std::to_string defaults to 6 decimal places
      // when given an input value of type float or double
      line = line + std::to_string(data[i][j]);
      if (j != column_num - 1) {
        line += delimiter;
      } else {
        line += "\n";
      }
    }
    write_outfile << line;
  }
  write_outfile.close();
}

// for Party::split_train_test_data
// save a copy of shuffled data_indexes vector<int> to file
// for local debugging
void write_shuffled_data_indexes_to_file(std::vector<int> data_indexes,
                                         const std::string &data_file) {
  std::ofstream write_outfile(data_file);
  if (!write_outfile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  // ostream_iterator for stream cout
  // with new line delimiter
  std::ostream_iterator<int> output_iterator(write_outfile, "\n");
  std::copy(data_indexes.begin(), data_indexes.end(), output_iterator);

  write_outfile.close();
}

std::string read_key_file(const std::string &key_file) {
  std::ifstream key_infile(key_file);
  if (!key_infile) {
    LOG(INFO) << "Open " << key_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string phe_keys_str;
  copy(std::istreambuf_iterator<char>(key_infile),
       std::istreambuf_iterator<char>(), back_inserter(phe_keys_str));
  key_infile.close();

  return phe_keys_str;
}

void write_key_to_file(std::string phe_keys_str, const std::string &key_file) {
  std::ofstream write_outfile(key_file);
  if (!write_outfile) {
    LOG(INFO) << "Open " << key_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  write_outfile << phe_keys_str;
  write_outfile.close();
}

void read_encoded_number_matrix_file(EncodedNumber **data_matrix, int row_num,
                                     int column_num,
                                     const std::string &encoded_number_file) {
  std::ifstream encoded_matrix_infile(encoded_number_file);
  if (!encoded_matrix_infile) {
    LOG(INFO) << "Open " << encoded_number_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string line;
  int row_idx = 0;
  while (std::getline(encoded_matrix_infile, line)) {
    std::istringstream ss(line);
    // decode base64 format to pb_string
    std::string pb_line = base64_decode_to_pb_string(line);
    // deserialize the line and get an EncodedNumber array
    auto *encoded_array = new EncodedNumber[column_num];
    deserialize_encoded_number_array(encoded_array, column_num, pb_line);
    // copy to data_matrix
    for (int j = 0; j < column_num; j++) {
      data_matrix[row_idx][j] = encoded_array[j];
    }
    delete[] encoded_array;
    row_idx++;
  }
  encoded_matrix_infile.close();
}

void write_encoded_number_matrix_to_file(
    EncodedNumber **data_matrix, int row_num, int column_num,
    const std::string &encoded_number_file) {
  std::ofstream encoded_matrix_outfile(encoded_number_file);
  if (!encoded_matrix_outfile) {
    LOG(INFO) << "Open " << encoded_number_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  log_info("[write_encoded_number_matrix_to_file] row_num = " +
           std::to_string(row_num));

  for (int i = 0; i < row_num; i++) {
    std::string line;
    // serialize encoded array to string and write
    serialize_encoded_number_array(data_matrix[i], column_num, line);
    // convert to base64 format
    std::string b64_line = base64_encode(
        reinterpret_cast<const BYTE *>(line.c_str()), line.size());
    b64_line += "\n";
    log_info("[write_encoded_number_matrix_to_file] write line " +
             std::to_string(i));
    encoded_matrix_outfile << b64_line;
  }
  encoded_matrix_outfile.close();
}

void read_encoded_number_array_file(EncodedNumber *data_arr, int row_num,
                                    const std::string &encoded_number_file) {
  std::ifstream encoded_arr_infile(encoded_number_file);
  if (!encoded_arr_infile) {
    LOG(INFO) << "Open " << encoded_number_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string line;
  int row_idx = 0;
  while (std::getline(encoded_arr_infile, line)) {
    std::istringstream ss(line);
    // decode base64 format to pb_string
    std::string pb_line = base64_decode_to_pb_string(line);
    // deserialize the line and get an EncodedNumber array
    auto *encoded_array = new EncodedNumber[1];
    deserialize_encoded_number_array(encoded_array, 1, pb_line);
    // copy to data_matrix
    data_arr[row_idx] = encoded_array[0];
    delete[] encoded_array;
    row_idx++;
  }
  encoded_arr_infile.close();
}

void write_encoded_number_array_to_file(
    EncodedNumber *data_arr, int row_num,
    const std::string &encoded_number_file) {
  std::ofstream encoded_arr_outfile(encoded_number_file);
  if (!encoded_arr_outfile) {
    LOG(INFO) << "Open " << encoded_number_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < row_num; i++) {
    std::string line;
    // serialize encoded array to string and write
    auto *encoded_arr = new EncodedNumber[1];
    encoded_arr[0] = data_arr[i];
    serialize_encoded_number_array(encoded_arr, 1, line);
    // convert to base64 format
    std::string b64_line = base64_encode(
        reinterpret_cast<const BYTE *>(line.c_str()), line.size());
    b64_line += "\n";
    encoded_arr_outfile << b64_line;
    delete[] encoded_arr;
  }
  encoded_arr_outfile.close();
}