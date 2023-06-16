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
// Created by root on 5/31/22.
//

#include <falcon/utils/alg/vec_util.h>
#include <falcon/utils/logger/logger.h>

std::vector<double>
flatten_2d_vector(const std::vector<std::vector<double>> &mat) {
  if (mat.empty()) {
    log_error("The 2d vector to be flattened is empty.");
    exit(EXIT_FAILURE);
  }
  int row_size = (int)mat.size();
  int column_size = (int)mat[0].size();
  std::vector<double> ret_vec;
  for (int i = 0; i < row_size; i++) {
    for (int j = 0; j < column_size; j++) {
      ret_vec.push_back(mat[i][j]);
    }
  }
  return ret_vec;
}

std::vector<std::vector<double>>
expend_1d_vector(const std::vector<double> &vec, int row_size,
                 int column_size) {
  if (vec.empty() || (vec.size() != row_size * column_size)) {
    log_error("The 1d vector to be expended is empty.");
    exit(EXIT_FAILURE);
  }
  std::vector<std::vector<double>> ret_mat;
  for (int i = 0; i < row_size; i++) {
    std::vector<double> row_vec;
    for (int j = 0; j < column_size; j++) {
      row_vec.push_back(vec[i * column_size + j]);
    }
    ret_mat.push_back(row_vec);
  }

  return ret_mat;
}

std::vector<std::vector<double>>
trans_mat(const std::vector<std::vector<double>> &mat) {
  if (mat.empty()) {
    log_error("The 2d vector to be transposed is empty.");
    exit(EXIT_FAILURE);
  }
  int row_size = (int)mat.size();
  int column_size = (int)mat[0].size();
  std::vector<std::vector<double>> trans_mat;
  for (int j = 0; j < column_size; j++) {
    std::vector<double> vec;
    vec.reserve(row_size);
    for (int i = 0; i < row_size; i++) {
      vec.push_back(mat[i][j]);
    }
    trans_mat.push_back(vec);
  }
  return trans_mat;
}