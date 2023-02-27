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
// Created by root on 4/23/22.
//

#include <falcon/party/info_exchange.h>
#include <falcon/utils/pb_converter/common_converter.h>

void send_int_array(const Party &party, const std::vector<int> &arr,
                    int dest_party_id) {
  std::string arr_str;
  serialize_int_array(arr, arr_str);
  party.send_long_message(dest_party_id, arr_str);
}

std::vector<int> recv_int_array(const Party &party, int src_party_id) {
  std::string recv_str;
  party.recv_long_message(src_party_id, recv_str);
  std::vector<int> recv_arr;
  deserialize_int_array(recv_arr, recv_str);
  return recv_arr;
}

void send_double_array(const Party &party, const std::vector<double> &arr,
                       int dest_party_id) {
  std::string arr_str;
  serialize_double_array(arr, arr_str);
  party.send_long_message(dest_party_id, arr_str);
}

std::vector<double> recv_double_array(const Party &party, int src_party_id) {
  std::string recv_str;
  party.recv_long_message(src_party_id, recv_str);
  std::vector<double> recv_arr;
  deserialize_double_array(recv_arr, recv_str);
  return recv_arr;
}

void send_encoded_number_array(const Party &party, EncodedNumber *arr, int size,
                               int dest_party_id) {
  std::string arr_str;
  serialize_encoded_number_array(arr, size, arr_str);
  party.send_long_message(dest_party_id, arr_str);
}

void recv_encoded_number_array(const Party &party, EncodedNumber *arr, int size,
                               int src_party_id) {
  std::string recv_str;
  party.recv_long_message(src_party_id, recv_str);
  deserialize_encoded_number_array(arr, size, recv_str);
}

void send_encoded_number_matrix(const Party &party, EncodedNumber **mat,
                                int row_size, int column_size,
                                int dest_party_id) {
  std::string mat_str;
  serialize_encoded_number_matrix(mat, row_size, column_size, mat_str);
  party.send_long_message(dest_party_id, mat_str);
}

void recv_encoded_number_matrix(const Party &party, EncodedNumber **mat,
                                int row_size, int column_size,
                                int src_party_id) {
  std::string recv_str;
  party.recv_long_message(src_party_id, recv_str);
  deserialize_encoded_number_matrix(mat, row_size, column_size, recv_str);
}

void broadcast_int_array(const Party &party, std::vector<int> &arr,
                         int req_party_id) {
  if (party.party_id == req_party_id) {
    // serialize the array to be broadcast
    std::string arr_str;
    serialize_int_array(arr, arr_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, arr_str);
      }
    }
  } else {
    // receive and deserialize the array
    std::string recv_arr_str;
    party.recv_long_message(req_party_id, recv_arr_str);
    deserialize_int_array(arr, recv_arr_str);
  }
}

void broadcast_double_array(const Party &party, std::vector<double> &arr,
                            int req_party_id) {
  if (party.party_id == req_party_id) {
    // serialize the array to be broadcast
    std::string arr_str;
    serialize_double_array(arr, arr_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, arr_str);
      }
    }
  } else {
    // receive and deserialize the array
    std::string recv_arr_str;
    party.recv_long_message(req_party_id, recv_arr_str);
    deserialize_double_array(arr, recv_arr_str);
  }
}

void broadcast_encoded_number_array(const Party &party, EncodedNumber *arr,
                                    int size, int req_party_id) {
  if (party.party_id == req_party_id) {
    // serialize the encoded number vector and send to other parties
    std::string arr_str;
    serialize_encoded_number_array(arr, size, arr_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, arr_str);
      }
    }
  } else {
    // receive the message from req_party_id and set to vec
    std::string recv_arr_str;
    party.recv_long_message(req_party_id, recv_arr_str);
    deserialize_encoded_number_array(arr, size, recv_arr_str);
  }
}

void broadcast_encoded_number_matrix(const Party &party, EncodedNumber **mat,
                                     int row_size, int column_size,
                                     int req_party_id) {
  if (party.party_id == req_party_id) {
    // serialize the encoded number matrix and send to other parties
    std::string mat_str;
    serialize_encoded_number_matrix(mat, row_size, column_size, mat_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, mat_str);
      }
    }
  } else {
    // receive the message from req_party_id and set to vec
    std::string recv_mat_str;
    party.recv_long_message(req_party_id, recv_mat_str);
    deserialize_encoded_number_matrix(mat, row_size, column_size, recv_mat_str);
  }
}

std::vector<int> sync_up_int_arr(const Party &party, int v) {
  std::vector<int> sync_arr;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // first set its own weight size and receive other parties' weight sizes
    sync_arr.push_back(v);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        std::string recv_weight_size;
        party.recv_long_message(i, recv_weight_size);
        sync_arr.push_back(std::stoi(recv_weight_size));
      }
    }
    // then broadcast the vector
    std::string party_weight_sizes_str;
    serialize_int_array(sync_arr, party_weight_sizes_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, party_weight_sizes_str);
      }
    }
  } else {
    // first send the weight size to active party
    party.send_long_message(ACTIVE_PARTY_ID, std::to_string(v));
    // then receive and deserialize the party_weight_sizes array
    std::string recv_party_weight_sizes_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_party_weight_sizes_str);
    deserialize_int_array(sync_arr, recv_party_weight_sizes_str);
  }
  return sync_arr;
}

std::vector<int> sync_up_int_arrs(const Party &party,
                                  const std::vector<int> &arr) {
  // first sync up each party's local arr size
  std::vector<int> arr_size = sync_up_int_arr(party, (int)arr.size());
  // then, sync up the int arr
  std::vector<int> flattened_arr;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self arr
    for (int v : arr) {
      flattened_arr.push_back(v);
    }
    // receive arr from other parties and append
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::vector<int> recv_arr_id = recv_int_array(party, id);
        for (int v : recv_arr_id) {
          flattened_arr.push_back(v);
        }
      }
    }
    // serialize and broadcast
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        send_int_array(party, flattened_arr, id);
      }
    }
  } else {
    // first send the int arr to active party
    send_int_array(party, arr, ACTIVE_PARTY_ID);
    // then receive and deserialize the flattened int array
    flattened_arr = recv_int_array(party, ACTIVE_PARTY_ID);
  }

  return flattened_arr;
}

std::vector<double> sync_up_double_arrs(const Party &party,
                                        const std::vector<double> &arr) {
  // first sync up each party's local arr size
  std::vector<int> arr_size = sync_up_int_arr(party, (int)arr.size());
  // then, sync up the double arr
  std::vector<double> flattened_arr;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self arr
    for (double v : arr) {
      flattened_arr.push_back(v);
    }
    // receive arr from other parties and append
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        std::vector<double> recv_arr_id = recv_double_array(party, id);
        for (double v : recv_arr_id) {
          flattened_arr.push_back(v);
        }
      }
    }
    // serialize and broadcast
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        send_double_array(party, flattened_arr, id);
      }
    }
  } else {
    // first send the double arr to active party
    send_double_array(party, arr, ACTIVE_PARTY_ID);
    // then receive and deserialize the flattened double array
    flattened_arr = recv_double_array(party, ACTIVE_PARTY_ID);
  }

  return flattened_arr;
}