//
// Created by root on 5/31/22.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_ALG_VEC_UTIL_H_
#define FALCON_INCLUDE_FALCON_UTILS_ALG_VEC_UTIL_H_

#include <vector>

/**
 * convert a 2d vector (matrix) into a 1d vector
 *
 * @param mat: the matrix to be converted
 * @return the converted 1d vector
 */
std::vector<double> flatten_2d_vector(const std::vector<std::vector<double>>& mat);

/**
 * expend a 1d vector into a 2d vector (matrix)
 *
 * @param vec: the 1d vector to be expended
 * @param row_size: the row size of the matrix to be expended
 * @param column_size: the column size of the matrix to be expended
 * @return
 */
std::vector<std::vector<double>> expend_1d_vector(const std::vector<double>& vec,
                                                  int row_size, int column_size);

#endif //FALCON_INCLUDE_FALCON_UTILS_ALG_VEC_UTIL_H_
