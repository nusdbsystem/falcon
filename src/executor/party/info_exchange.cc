//
// Created by root on 4/23/22.
//

#include <falcon/party/info_exchange.h>
#include <falcon/utils/pb_converter/common_converter.h>

void broadcast_encoded_number_array(const Party&party, EncodedNumber *vec,
                                    int size, int req_party_id) {
  if (party.party_id == req_party_id) {
    // serialize the encoded number vector and send to other parties
    std::string vec_str;
    serialize_encoded_number_array(vec, size, vec_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, vec_str);
      }
    }
  } else {
    // receive the message from req_party_id and set to vec
    std::string recv_vec_str;
    party.recv_long_message(req_party_id, recv_vec_str);
    deserialize_encoded_number_array(vec, size, recv_vec_str);
  }
}

std::vector<int> sync_up_int_arr(const Party&party, int v) {
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