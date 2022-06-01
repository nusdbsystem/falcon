//
// Created by root on 5/31/22.
//

#include <falcon/utils/alg/vec_util.h>
#include <falcon/utils/logger/logger.h>

std::vector<double> flatten_2d_vector(const std::vector<std::vector<double>>& mat) {
  if (mat.empty()) {
    log_error("The 2d vector to be flattened is empty.");
    exit(EXIT_FAILURE);
  }
  int row_size = (int) mat.size();
  int column_size = (int) mat[0].size();
  std::vector<double> ret_vec;
  for (int i = 0; i < row_size; i++) {
    for (int j = 0; j < column_size; j++) {
      ret_vec.push_back(mat[i][j]);
    }
  }
  return ret_vec;
}

std::vector<std::vector<double>> expend_1d_vector(const std::vector<double>& vec,
                                                  int row_size, int column_size) {
  if (vec.empty() || (vec.size() != row_size * column_size)) {
    log_error("The 1d vector to be expended is empty.");
    exit(EXIT_FAILURE);
  }
  std::vector<std::vector<double>> ret_mat;
  for (int i = 0; i < row_size; i++) {
    std::vector<double> row_vec;
    row_vec.reserve(column_size);
    for (int j = 0; j < column_size; j++) {
      row_vec.push_back(vec[i * row_size + j]);
    }
    ret_mat.push_back(row_vec);
  }

  return ret_mat;
}