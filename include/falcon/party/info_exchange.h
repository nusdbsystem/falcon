//
// Created by root on 4/23/22.
//

#ifndef FALCON_INCLUDE_FALCON_PARTY_INFO_EXCHANGE_H_
#define FALCON_INCLUDE_FALCON_PARTY_INFO_EXCHANGE_H_

#include <falcon/party/party.h>
#include <falcon/operator/phe/fixed_point_encoder.h>

/**
 * broadcast an encoded vector to other parties
 *
 * @param party: the participating party
 * @param vec: the encoded vector to be broadcast
 * @param size: the size of the vector
 * @param req_party_id: the party who has the vector to be broadcast
 */
void broadcast_encoded_number_array(const Party&party, EncodedNumber *vec,
                                    int size, int req_party_id);

/**
 * each party has a int value, sync up to obtain a int array
 *
 * @param v: the value to be sync up
 * @return
 */
std::vector<int> sync_up_int_arr(const Party& party, int v);


#endif //FALCON_INCLUDE_FALCON_PARTY_INFO_EXCHANGE_H_
