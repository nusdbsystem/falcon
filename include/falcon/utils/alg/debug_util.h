//
// Created by root on 3/12/22.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_ALG_DEBUG_UTIL_H_
#define FALCON_INCLUDE_FALCON_UTILS_ALG_DEBUG_UTIL_H_

#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/party/party.h>
#include <falcon/utils/logger/logger.h>

/**
 * This function implements debug of cipher array for check
 *
 * @tparam T: the template of the plaintext, either double or int
 * @param party: the participating party
 * @param cipher_array: the cipher array to be debug
 * @param size: the size of cipher_array
 * @param req_party_id: the request party id
 * @param print_flag: whether print the debug information inside the function
 * @param print_size: the size for printing the debug information, should <= size
 * @return
 */
template <typename T>
std::vector<T> debug_cipher_array(const Party& party, EncodedNumber* cipher_array,
                                  int size, int req_party_id,
                                  bool print_flag = false, int print_size = 0) {
  // request party broadcast the cipher array
  party.broadcast_encoded_number_array(cipher_array, size, req_party_id);
  // call collaborative decrypt function
  auto *plain_array = new EncodedNumber[size];
  party.collaborative_decrypt(cipher_array, plain_array, size, req_party_id);
  // request party should broadcast the plain array
  party.broadcast_encoded_number_array(plain_array, size, req_party_id);
  // decode the values and return
  std::vector<T> decoded_array;
  for (int i = 0; i < size; i++) {
    T t;
    plain_array[i].decode(t);
    decoded_array.push_back(t);
  }
  // print the debug information
  if (print_flag) {
    for (int i = 0; i < print_size; i++) {
      log_info("[debug_cipher_array] decoded_array[" + std::to_string(i)
                   + "] = " + std::to_string(decoded_array[i]));
    }
  }
  delete [] plain_array;
  return decoded_array;
}

/**
 * This function implements debug of cipher matrix for check
 *
 * @tparam T: the template of the plaintext, either double or int
 * @param party: the participating party
 * @param cipher_matrix: the cipher matrix to be debug
 * @param row: the number of rows in cipher_matrix
 * @param column: the number of columns in cipher_matrix
 * @param req_party_id: the request party id
 * @param print_flag: whether print the debug information inside the function
 * @param print_row: the size for printing the debug information, should <= row
 * @param print_column: the size for printing the debug information, should <= column
 * @return
 */
template <typename T>
std::vector<std::vector<T>> debug_cipher_matrix(const Party& party, EncodedNumber** cipher_matrix,
                                                int row, int column, int req_party_id,
                                                bool print_flag = false, int print_row = 0, int print_column = 0)  {
  // iteratively debug cipher array
  std::vector<std::vector<T>> decoded_matrix;
  for (int i = 0; i < row; i++) {
    std::vector<T> decoded_array = debug_cipher_array<T>(party, cipher_matrix[i], column, req_party_id);
    decoded_matrix.push_back(decoded_array);
  }
  // print the debug information
  if (print_flag) {
    for (int i = 0; i < print_row; i++) {
      for (int j = 0; j < print_column; j++) {
        log_info("[debug_cipher_matrix] debug_cipher_matrix[" + std::to_string(i)
                     + "][ = " + std::to_string(j) + "] = " + std::to_string(decoded_matrix[i][j]));
      }
    }
  }
  return decoded_matrix;
}

#endif //FALCON_INCLUDE_FALCON_UTILS_ALG_DEBUG_UTIL_H_
