//
// Created by root on 4/21/22.
//

#ifndef FALCON_INCLUDE_FALCON_OPERATOR_CONVERSION_OP_CONV_H_
#define FALCON_INCLUDE_FALCON_OPERATOR_CONVERSION_OP_CONV_H_

#include <falcon/party/party.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/operator/mpc/spdz_connector.h>

/**
   * parties jointly decrypt a ciphertext vector,
   * assume that the parties have already have the same src_ciphers.
   * The request party obtains the decrypted plaintext while
   * the other parties obtain nothing plaintext.
   *
   * @param party: the participating party
   * @param src_ciphers: ciphertext vector to be decrypted
   * @param dest_plains: decrypted plaintext vector
   * @param size: size of the vector
   * @param req_party_id: party that initiate decryption
   */
void collaborative_decrypt(const Party& party, EncodedNumber* src_ciphers,
                           EncodedNumber* dest_plains, int size, int req_party_id);

/**
 * convert ciphertext vector to secret shares securely,
 * Algorithm 1: Conversion to secretly shared value in paper
 * <Privacy Preserving Vertical Federated Learning for Tree-based Models>
 *
 * @param party: the participating party
 * @param src_ciphers: ciphertext vector to be decrypted
 * @param secret_shares: decrypted and decoded secret shares
 * @param size: size of the vector
 * @param req_party_id: party that initiate decryption
 * @param phe_precision: fixed point precision when encoding
 */
void ciphers_to_secret_shares(const Party& party, EncodedNumber* src_ciphers,
                              std::vector<double>& secret_shares, int size,
                              int req_party_id, int phe_precision);

/**
 * convert secret shares back to ciphertext vector
 *
 * @param party: the participating party
 * @param dest_ciphers: ciphertext vector to be recovered
 * @param secret_shares: secret shares received from spdz parties
 * @param size: size of the vector
 * @param req_party_id: party that initiate conversion
 * @param phe_precision: ciphertext vector precision, need careful design
 */
void secret_shares_to_ciphers(const Party& party, EncodedNumber* dest_ciphers,
                              std::vector<double> secret_shares, int size,
                              int req_party_id, int phe_precision);

/**
 * truncate the ciphertext precision to a lower one
 *
 * @param party: the participating party
 * @param ciphers: the ciphertexts to be truncated precision
 * @param size: the size of the vector
 * @param req_party_id: the party who has the ciphertexts
 * @param dest_precision: the destination precision
 */
void truncate_ciphers_precision(const Party& party, EncodedNumber *ciphers, int size,
                                int req_party_id, int dest_precision);

/**
   * compute the multiplication of two cipher vectors
   *
   * @param party: the participating party
   * @param res: the resulted cipher vector
   * @param ciphers1: the first cipher vector
   * @param ciphers2: the second cipher vector
   * @param size: the size of the two vectors
   * @param req_party_id: the party who request the multiplication
   */
void ciphers_multi(const Party& party, EncodedNumber* res,
                   EncodedNumber* ciphers1, EncodedNumber* ciphers2,
                   int size, int req_party_id);

#endif //FALCON_INCLUDE_FALCON_OPERATOR_CONVERSION_OP_CONV_H_
