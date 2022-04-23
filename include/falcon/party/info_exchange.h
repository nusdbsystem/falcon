//
// Created by root on 4/23/22.
//

#ifndef FALCON_INCLUDE_FALCON_PARTY_INFO_EXCHANGE_H_
#define FALCON_INCLUDE_FALCON_PARTY_INFO_EXCHANGE_H_

#include <falcon/party/party.h>
#include <falcon/operator/phe/fixed_point_encoder.h>

/**
 * send a int array to dest_party_id
 *
 * @param party: the participating party
 * @param arr: array to be sent
 * @param dest_party_id: the party id to be sent
 */
void send_int_array(const Party& party, const std::vector<int>& arr, int dest_party_id);

/**
 * recv a int array from src_party_id
 *
 * @param party: the participating party
 * @param src_party_id: the party that sends the array
 * @return
 */
std::vector<int> recv_int_array(const Party& party, int src_party_id);

/**
 * send a double array to dest_party_id
 *
 * @param party: the participating party
 * @param arr: array to be sent
 * @param dest_party_id: the party id to be sent
 */
void send_double_array(const Party& party, const std::vector<double>& arr, int dest_party_id);

/**
 * recv a double array from src_party_id
 *
 * @param party: the participating party
 * @param src_party_id: the party that sends the array
 * @return
 */
std::vector<double> recv_double_array(const Party& party, int src_party_id);

/**
 * send an encoded number array to dest_party_id
 *
 * @param party: the participating party
 * @param arr: encoded number array to be sent
 * @param size: the size of the array
 * @param dest_party_id: the party id to be sent
 */
void send_encoded_number_array(const Party& party, EncodedNumber* arr, int size, int dest_party_id);

/**
 * recv an encoded number array from src_party_id
 *
 * @param party: the participating party
 * @param arr: encoded number array to be sent
 * @param size: the size of the array
 * @param src_party_id: the party that sends the array
 * @return
 */
void recv_encoded_number_array(const Party& party, EncodedNumber* arr, int size, int src_party_id);

/**
 * send an encoded number matrix to dest_party_id
 *
 * @param party: the participating party
 * @param mat: matrix to be sent
 * @param row_size: the number of rows
 * @param column_size: the number of columns
 * @param dest_party_id: the party id to be sent
 */
void send_encoded_number_matrix(const Party& party, EncodedNumber** mat,
                                int row_size, int column_size, int dest_party_id);

/**
 * recv an encoded number matrix from src_party_id
 *
 * @param party: the participating party
 * @param mat: matrix to be received
 * @param row_size: the number of rows
 * @param column_size: the number of columns
 * @param src_party_id: the party that sends the matrix
 */
void recv_encoded_number_matrix(const Party& party, EncodedNumber** mat,
                                int row_size, int column_size, int src_party_id);
/**
 * broadcast an int array to other parties
 *
 * @param party: the participating party
 * @param arr: the int array to be broadcast
 * @param req_party_id: the party who has the vector to be broadcast
 */
void broadcast_int_array(const Party& party, std::vector<int>& arr, int req_party_id);

/**
 * broadcast an double array to other parties
 *
 * @param party: the participating party
 * @param arr: the double array to be broadcast
 * @param req_party_id: the party who has the vector to be broadcast
 */
void broadcast_double_array(const Party& party, std::vector<double>& arr, int req_party_id);

/**
 * broadcast an encoded vector to other parties
 *
 * @param party: the participating party
 * @param arr: the encoded vector to be broadcast
 * @param size: the size of the vector
 * @param req_party_id: the party who has the vector to be broadcast
 */
void broadcast_encoded_number_array(const Party& party, EncodedNumber *arr,
                                    int size, int req_party_id);

/**
 * broadcast an encoded matrix to other parties
 *
 * @param party: the participating party
 * @param mat: the encoded number matrix to be broadcast
 * @param row_size: the number of rows
 * @param column_size: the number of columns
 * @param req_party_id: the party who has the vector to be broadcast
 */
void broadcast_encoded_number_matrix(const Party& party, EncodedNumber** mat,
                                     int row_size, int column_size, int req_party_id);

/**
 * each party has a int value, sync up to obtain a int array
 * (This function is mainly for sync up party's local weight size)
 *
 * @param party: the participating party
 * @param v: the value to be sync up
 * @return
 */
std::vector<int> sync_up_int_arr(const Party& party, int v);

/**
 * each party has a int array, sync up to obtain a flatten int array
 *
 * @param party: the participating party
 * @param arr: the int array to be sync up
 * @return
 */
std::vector<int> sync_up_int_arrs(const Party& party, const std::vector<int>& arr);

/**
 * each party has a double array, sync up to obtain a flatten double array
 *
 * @param party: the participating party
 * @param arr: the double array to be sync up
 * @return
 */
std::vector<double> sync_up_double_arrs(const Party& party, const std::vector<double>& arr);

#endif //FALCON_INCLUDE_FALCON_PARTY_INFO_EXCHANGE_H_
