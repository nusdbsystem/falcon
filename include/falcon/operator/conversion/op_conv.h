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
void collaborative_decrypt(const Party &party,
                           EncodedNumber *src_ciphers,
                           EncodedNumber *dest_plains,
                           int size,
                           int req_party_id);

/**
 * convert 1-dimension ciphertext vector to secret shares securely, each party will have a share
 * Algorithm 1: Conversion to secretly shared value in paper
 * <Privacy Preserving Vertical Federated Learning for Tree-based Models>
 *
 * @param party: the participating party
 * @param src_ciphers: ciphertext vector to be decrypted, hold by one party
 * @param secret_shares: decrypted and decoded secret shares
 * @param size: size of the vector
 * @param req_party_id: party that initiate decryption
 * @param phe_precision: fixed point precision when encoding
 */
void ciphers_to_secret_shares(const Party &party,
                              EncodedNumber *src_ciphers,
                              std::vector<double> &secret_shares,
                              int size,
                              int req_party_id,
                              int phe_precision);

/**
 * convert 2-dimension ciphertext matrix to 2-dimensional secret shares
 *
 * @param party: the participating party
 * @param src_ciphers_mat: ciphertext matrix to be decrypted
 * @param secret_shares_mat: decrypted and decoded secret shares matrix
 * @param row_size: row size of the matrix
 * @param column_size: column size of the matrix
 * @param req_party_id: party that initiate decryption
 * @param phe_precision: fixed point precision when encoding
 */
void ciphers_mat_to_secret_shares_mat(const Party &party, EncodedNumber **src_ciphers_mat,
                                      std::vector<std::vector<double>> &secret_shares_mat,
                                      int row_size,
                                      int column_size,
                                      int req_party_id,
                                      int phe_precision);

/**
 * Convert secret shares back to ciphertext vector,
 * After Converting, all parties will have the ciphertext
 *
 * @param party: the participating party
 * @param dest_ciphers: ciphertext vector to be recovered
 * @param secret_shares: secret shares received from spdz parties
 * @param size: size of the vector
 * @param req_party_id: party that initiate conversion
 * @param phe_precision: ciphertext vector precision, need careful design
 */
void secret_shares_to_ciphers(const Party &party,
                              EncodedNumber *dest_ciphers,
                              std::vector<double> secret_shares,
                              int size,
                              int req_party_id,
                              int phe_precision);

/**
 * Convert secret shares back to plaintext vector,
 * After Converting, all parties will have the ciphertext
 *
 * @param party: the participating party
 * @param dest_plaintext_double: plaintext of vectors
 * @param secret_shares: secret shares received from spdz parties
 * @param size: size of the vector
 * @param req_party_id: party that initiate conversion
 * @param phe_precision: ciphertext vector precision, need careful design
 */
void secret_shares_to_plain_double(const Party &party, vector<double> &dest_plaintext_double,
                                   std::vector<double> secret_shares,
                                   int size, int req_party_id, int phe_precision);

/***********************************************************/
/***************** cipher & share operations ***************/
/***********************************************************/

/**
 * truncate the ciphertext precision to a lower one
 *
 * @param party: the participating party
 * @param ciphers: the ciphertexts to be truncated precision
 * @param size: the size of the vector
 * @param req_party_id: the party who has the ciphertexts
 * @param dest_precision: the destination precision
 */
void truncate_ciphers_precision(const Party &party,
                                EncodedNumber *ciphers,
                                int size,
                                int req_party_id,
                                int dest_precision);

/**
 * compute the element-wise multiplication of two cipher vectors.
 * each party should have cipher2. cipher is converted into shares
 * eg,. {[a1], [a2], [a3]} * {[b1], [b2], [b3]} = {[a1b1], [a2b2], [a3b3]}
 * {[a1], [a2], [a3]} => secret shares with
 *      party1 have {<a1_share1>, <a1_share1>, <a1_share1>}
 *      party2 have {<a1_share2>, <a1_share2>, <a1_share2>}
 *      party3 have {<a1_share3>, <a1_share3>, <a1_share3>}
 * each party calculate multiplication between local share and b
 * eg,. party 1 calculate {<a1_share1>, <a1_share1>, <a1_share1>} * {[b1], [b2], [b3]}
 * finally, all party send local res to req_party_id party to aggregate
 *
 * @param party: the participating party
 * @param res: the resulted cipher vector
 * @param ciphers1: the first cipher vector
 * @param ciphers2: the second cipher vector
 * @param size: the size of the two vectors
 * @param req_party_id: the party who request the multiplication
 */
void ciphers_ele_wise_multi(const Party &party,
                            EncodedNumber *res,
                            EncodedNumber *ciphers1,
                            EncodedNumber *ciphers2,
                            int size,
                            int req_party_id);
/**
 * transpose an encoded matrix
 *
 * @param source_mat: the source encoded matrix in form of i*j
 * @param n_source_row: the number of rows in source matrix, eg i
 * @param n_source_col: the number of columns in source matrix, eg, j
 * @param ret_mat: the returned encoded matrix, eg, in form of j*i
 */
void transpose_encoded_mat(EncodedNumber **source_mat,
                           int n_source_row,
                           int n_source_col,
                           EncodedNumber **ret_mat);

/**
 * MatShareMul: This function compute the cipher and secret shares multiplication
 * each party holds a secret share matrix and the same ciphers
 * eg,. share(M*N) * cipher(N*K) = RES(M*K)
 *
 * @param party: initialized party object
 * @param shares: secret shares that held by parties
 * @param ciphers: the common ciphers for multiplication, all party must have this,
 * @param n_shares_row: the number of rows in shares
 * @param n_shares_col: the number columns in shares
 * @param n_ciphers_row: the number of rows in ciphers
 * @param n_ciphers_col: the number of columns in ciphers
 * @param ret: the returned result, dim = (n_shares_col, n_ciphers_col)
 */
void cipher_shares_mat_mul(const Party &party,
                           const std::vector<std::vector<double>> &shares,
                           EncodedNumber **ciphers,
                           int n_shares_row,
                           int n_shares_col,
                           int n_ciphers_row,
                           int n_ciphers_col,
                           EncodedNumber **ret);

/**
 * This function compute the element-wise multiplication between cipher and secret shares
 * each party holds a secret share matrix and the same ciphers
 *
 * @param party: initialized party object
 * @param shares: secret shares that held by parties
 * @param ciphers: the common ciphers for element-wise multiplication
 * @param size: the number of element in ciphers
 * @param ret: the returned result, dim = (n_shares_col, n_ciphers_col)
 */
void cipher_shares_ele_wise_vec_mul(const Party &party,
                                    const std::vector<double> &shares,
                                    EncodedNumber *ciphers,
                                    int size,
                                    EncodedNumber *ret);

/**
 * Active party gather all share vector "vec" from others and aggregate
 * @param party: initialized party object
 * @param vec secret share
 * @return agg_mat
 */
std::vector<double> display_shares_vector(
    const Party &party,
    const std::vector<double> &vec);

/**
 * Active party gather all share metrics "mat" from others and aggregate
 * @param party: initialized party object
 * @param vec secret share
 * @return agg_mat
 */
std::vector<std::vector<double>> display_shares_matrix(
    const Party &party,
    const std::vector<std::vector<double>> &mat);


void cipher_share_mul(const Party &party,
                      const double &share,
                      const EncodedNumber &cipher,
                      EncodedNumber &ret
);
#endif //FALCON_INCLUDE_FALCON_OPERATOR_CONVERSION_OP_CONV_H_
