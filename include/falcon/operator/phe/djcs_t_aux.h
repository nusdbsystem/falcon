//
// Created by wuyuncheng on 30/10/20.
//

#ifndef FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_
#define FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_

#include "falcon/operator/phe/fixed_point_encoder.h"
#include "gmp.h"    // gmp is included implicitly
#include "libhcs.h" // master header includes everything
#include <vector>

/***********************************************************/
/*************** individual number operations *************/
/***********************************************************/

/**
 * encrypt a plaintext EncodedNumber
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: ciphertext EncodedNumber
 * @param plain: plaintext EncodedNumber
 */
void djcs_t_aux_encrypt(djcs_t_public_key *pk, hcs_random *hr,
                        EncodedNumber &res, const EncodedNumber &plain);

/**
 * decrypt a ciphertext EncodedNumber
 *
 * @param pk: public key
 * @param au: decryption authenticated server
 * @param res: partially decrypted EncodedNumber
 * @param cipher: ciphertext EncodedNumber
 */
void djcs_t_aux_partial_decrypt(djcs_t_public_key *pk, djcs_t_auth_server *au,
                                EncodedNumber &res,
                                const EncodedNumber &cipher);

/**
 * combine partially decrypted EncodedNumbers
 *
 * @param pk: public key
 * @param res: decrypted EncodedNumber
 * @param shares: partially decrypted EncodedNumbers
 * @param size: size of the shares vector
 */
void djcs_t_aux_share_combine(djcs_t_public_key *pk, EncodedNumber &res,
                              EncodedNumber *shares, int size);

/**
 * homomorphic addition of two ciphers and return an EncodedNumber,
 * both cipher have same exponent
 *
 * @param pk: public key
 * @param res: summation ciphertext
 * @param cipher1: first ciphertext EncodedNumber
 * @param cipher2: second ciphertext EncodedNumber
 */
void djcs_t_aux_ee_add(djcs_t_public_key *pk, EncodedNumber &res,
                       const EncodedNumber &cipher1,
                       const EncodedNumber &cipher2);

/**
 * Add all values in a vector into a single EncodedNumber
 *
 * @param pk: public key
 * @param res: added ciphertext
 * @param size: size of the vector
 * @param cipher_vector: the vector to be aggregated
 */
void djcs_t_aux_ee_add_a_vector(djcs_t_public_key *pk, hcs_random *hr,
                                EncodedNumber &res, int size,
                                EncodedNumber *cipher_vector);

/**
 * homomorphic addition of two ciphers and return an EncodedNumber
 * both cipher have un-identical exponents,
 * increase one cipher's exponent to match
 *
 * @param pk: public key
 * @param res: summation ciphertext
 * @param cipher1: first ciphertext EncodedNumber
 * @param cipher2: second ciphertext EncodedNumber
 */
void djcs_t_aux_ee_add_ext(djcs_t_public_key *pk, EncodedNumber &res,
                           EncodedNumber cipher1, EncodedNumber cipher2);

/**
 * homomorphic multiplication of a cipher and a plain
 * return an EncodedNumber
 *
 * @param pk: public key
 * @param res: multiplication ciphertext
 * @param cipher: ciphertext EncodedNumber
 * @param plain: plaintext EncodedNumber
 */
void djcs_t_aux_ep_mul(djcs_t_public_key *pk, EncodedNumber &res,
                       const EncodedNumber &cipher, const EncodedNumber &plain);

/**
 * this function increases the precision of a ciphertext,
 * target_precision should be greater tha precision(exponent) of cipher
 * for computing homomorphic addition when the precision does not match
 *
 * @param pk: public key
 * @param res: multiplication ciphertext
 * @param target_precision: the precision to be reached
 * @param cipher: ciphertext EncodedNumber
 */
void djcs_t_aux_increase_prec(djcs_t_public_key *pk, EncodedNumber &res,
                              int target_precision, EncodedNumber cipher);

/***********************************************************/
/***************** vector related operations ***************/
/***********************************************************/

/**
 * encrypt a double vector based on pk and phe precision
 *
 * @param pk: public key
 * @param hr: djcs_t random
 * @param res: resulted ciphertext vector
 * @param size: the size of the vector
 * @param vec: values to be encrypted
 * @param phe_precision: the precision to be used
 */
void djcs_t_aux_double_vec_encryption(
    djcs_t_public_key *pk, hcs_random *hr, EncodedNumber *res, int size,
    const std::vector<double> &vec,
    int phe_precision = PHE_FIXED_POINT_PRECISION);

/**
 * encrypt a int vector based on pk and phe precision
 *
 * @param pk: public key
 * @param hr: djcs_t random
 * @param res: resulted ciphertext vector
 * @param size: the size of the vector
 * @param vec: values to be encrypted
 * @param phe_precision: the precision to be used
 */
void djcs_t_aux_int_vec_encryption(
    djcs_t_public_key *pk, hcs_random *hr, EncodedNumber *res, int size,
    const std::vector<int> &vec, int phe_precision = PHE_FIXED_POINT_PRECISION);

/**
 * homomorphic aggregate a cipher vector and return the result
 *
 * @param pk: public key
 * @param res: aggregated ciphertext
 * @param ciphers: a vector of ciphertext EncodedNumber
 * @param size: the size of the ciphers vector
 */
void djcs_t_aux_vec_aggregate(djcs_t_public_key *pk, EncodedNumber &res,
                              EncodedNumber *ciphers, int size);

/**
 * element-wise homomorphic ciphertext add
 * both cipher have same exponent
 *
 * @param pk: public key
 * @param res: the resulted cipher vector
 * @param ciphers1: the cipher vector 1
 * @param ciphers2: the cipher vector 2
 * @param size: the size of the two cipher vectors
 */
void djcs_t_aux_vec_ele_wise_ee_add(djcs_t_public_key *pk, EncodedNumber *res,
                                    EncodedNumber *ciphers1,
                                    EncodedNumber *ciphers2, int size);

/**
 * element-wise homomorphic ciphertext add
 * both cipher have un-identical exponents,
 * increase low-exponent cipher vector's exponent to match
 *
 * @param pk: public key
 * @param res: the resulted cipher vector
 * @param ciphers1: the cipher vector 1
 * @param ciphers2: the cipher vector 2
 * @param size: the size of the two cipher vectors
 */
void djcs_t_aux_vec_ele_wise_ee_add_ext(djcs_t_public_key *pk,
                                        EncodedNumber *res,
                                        EncodedNumber *ciphers1,
                                        EncodedNumber *ciphers2, int size);

/**
 * homomorphic inner product of a cipher vector and a plain vector, return an
 * EncodedNumber e,g. {a1, a2, a3 } * {[b1], [b2], [b3]} = [a1b1+a2b3+a3b3]
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: inner product ciphertext EncodedNumber
 * @param ciphers: a vector of ciphertext EncodedNumber
 * @param plains: a vector of plaintext EncodedNumber
 * @param size: vector size
 */
void djcs_t_aux_inner_product(djcs_t_public_key *pk, hcs_random *hr,
                              EncodedNumber &res, EncodedNumber *ciphers,
                              EncodedNumber *plains, int size);

/**
 * element-wise ciphertext plaintext multiplication
 * e,g. { a1, a2, a3 } * { [b1], [b2], [b3] } = { [a1b1], [a2b3], [a3b3] }
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: resulted ciphertext vector
 * @param ciphers: a vector of ciphertext EncodedNumber
 * @param plains: a vector of plaintext EncodedNumber
 * @param size: vector size
 */
void djcs_t_aux_vec_ele_wise_ep_mul(djcs_t_public_key *pk, EncodedNumber *res,
                                    EncodedNumber *ciphers,
                                    EncodedNumber *plains, int size);

/**
 * this function increases the precision of each element of a ciphertext vector,
 * for computing homomorphic addition when the precision does not match
 *
 * @param pk: public key
 * @param res: resulted ciphertext vector
 * @param target_precision: the precision to be reached
 * @param ciphers: the ciphertext vector
 * @param size: the size of the ciphertext vector
 */
void djcs_t_aux_increase_prec_vec(djcs_t_public_key *pk, EncodedNumber *res,
                                  int target_precision, EncodedNumber *ciphers,
                                  int size);

/***********************************************************/
/***************** matrix related operations ***************/
/***********************************************************/

/**
 * encrypt a double matrix based on pk and phe precision
 *
 * @param pk: public key
 * @param hr: djcs_t random
 * @param res: resulted ciphertext matrix
 * @param row_size: the row size of the matrix
 * @param column_size: the column size of the matrix
 * @param mat: values to be encrypted
 * @param phe_precision: the precision to be used
 */
void djcs_t_aux_double_mat_encryption(
    djcs_t_public_key *pk, hcs_random *hr, EncodedNumber **res, int row_size,
    int column_size, const std::vector<std::vector<double>> &mat,
    int phe_precision = PHE_FIXED_POINT_PRECISION);

/**
 * element-wise homomorphic addition of two cipher matrices
 *
 * @param pk: public key
 * @param res: resulted cipher matrix
 * @param cipher_mat1: the first cipher matrix
 * @param cipher_mat2: the second cipher matrix
 * @param row_size: the number of rows in the matrix
 * @param column_size: the number of columns in the matrix
 */
void djcs_t_aux_matrix_ele_wise_ee_add(djcs_t_public_key *pk,
                                       EncodedNumber **res,
                                       EncodedNumber **cipher_mat1,
                                       EncodedNumber **cipher_mat2,
                                       int row_size, int column_size);

/**
 * element-wise homomorphic addition of two cipher matrices
 * if the exponents are not identical, increase one cipher matrix's exponent to
 * match
 *
 * @param pk: public key
 * @param res: resulted cipher matrix
 * @param cipher_mat1: the first cipher matrix
 * @param cipher_mat2: the second cipher matrix
 * @param row_size: the number of rows in the matrix
 * @param column_size: the number of columns in the matrix
 */
void djcs_t_aux_matrix_ele_wise_ee_add_ext(djcs_t_public_key *pk,
                                           EncodedNumber **res,
                                           EncodedNumber **cipher_mat1,
                                           EncodedNumber **cipher_mat2,
                                           int row_size, int column_size);

/**
 * MatMul: homomorphic multiplication between a cipher vector and a plain
 * matrix, the result is a cipher vector size of cipher == column size of plain
 * @param pk: public key
 * @param hr: random variable
 * @param res: multiplication results with row_size [ae1+be2, ce1+de2]
 * @param ciphers: cipher vector, eg: [e1, e2]
 * @param plains: plain matrix, eg: [[a, b], [c, d]]
 * @param row_size: number of plaintext rows
 * @param column_size: number of plaintext columns, equal to cipher size
 */
void djcs_t_aux_vec_mat_ep_mult(djcs_t_public_key *pk, hcs_random *hr,
                                EncodedNumber *res, EncodedNumber *ciphers,
                                EncodedNumber **plains, int row_size,
                                int column_size);

/**
 * the homomorphic multiplication between a plaintext matrix and a ciphertext
 * matrix, the result is a cipher matrix need to ensure plain_column_size =
 * cipher_row_size e,g. plainText (M*N) * cipherText (N*K) = Res (M*K)
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: the resulted matrix of matrix multiplication
 * @param cipher_mat: the ciphertext matrix
 * @param plain_mat: the plaintext matrix
 * @param cipher_row_size: the number of rows of ciphertext matrix
 * @param cipher_column_size: the number of columns of ciphertext matrix
 * @param plain_row_size: the number of rows of plaintext matrix
 * @param plain_column_size: the number of columns of plaintext matrix
 */
void djcs_t_aux_mat_mat_ep_mult(djcs_t_public_key *pk, hcs_random *hr,
                                EncodedNumber **res, EncodedNumber **cipher_mat,
                                EncodedNumber **plain_mat, int cipher_row_size,
                                int cipher_column_size, int plain_row_size,
                                int plain_column_size);

/**
 * the element-wise matrix-matrix multiplication, need to ensure that
 * cipher_row_size = plain_row_size && cipher_column_size = plain_column_size
 *
 * @param pk: public key
 * @param res: the resulted matrix of matrix multiplication
 * @param cipher_mat: the ciphertext matrix
 * @param plain_mat: the plaintext matrix
 * @param cipher_row_size: the number of rows of ciphertext matrix
 * @param cipher_column_size: the number of columns of ciphertext matrix
 * @param plain_row_size: the number of rows of plaintext matrix
 * @param plain_column_size: the number of columns of plaintext matrix
 */
void djcs_t_aux_ele_wise_mat_mat_ep_mult(
    djcs_t_public_key *pk, EncodedNumber **res, EncodedNumber **cipher_mat,
    EncodedNumber **plain_mat, int cipher_row_size, int cipher_column_size,
    int plain_row_size, int plain_column_size);

/**
 * this function increases the precision of a ciphertext matrix,
 * for computing homomorphic addition when the precision does not match
 *
 * @param pk: public key
 * @param res: resulted ciphertext vector
 * @param target_precision: the precision to be reached
 * @param cipher_mat: the ciphertext matrix
 * @param row_size: the number of rows in the matrix
 * @param column_size: the number of columns in the matrix
 */
void djcs_t_aux_increase_prec_mat(djcs_t_public_key *pk, EncodedNumber **res,
                                  int target_precision,
                                  EncodedNumber **cipher_mat, int row_size,
                                  int column_size);

/**
 * copy a djcs_t public key from src to dest
 *
 * @param src: src public key object
 * @param dest: dest public key object
 */
void djcs_t_public_key_copy(djcs_t_public_key *src, djcs_t_public_key *dest);

/**
 * copy a djcs_t authenticate server from src to dest
 *
 * @param src: src auth server object
 * @param dest: dest auth server object
 */
void djcs_t_auth_server_copy(djcs_t_auth_server *src, djcs_t_auth_server *dest);

/**
 * copy a hcs_random from src to dest
 *
 * @param src: src hcs random object
 * @param dest: dest hcs random object
 */
void djcs_t_hcs_random_copy(hcs_random *src, hcs_random *dest);

/**
 * check a vector's size is not zero
 *
 * @param size: the size of the plain or cipher vector
 */
void check_size(int size);

/**
 * check if the two EncodedNumber have the same public key
 *
 * @param num1: the first EncodedNumber
 * @param num2: the second EncodedNumber
 */
void check_encoded_public_key(const EncodedNumber &num1,
                              const EncodedNumber &num2);

/**
 * check the two ciphertexts have the same exponent
 *
 * @param num1: the first ciphertext
 * @param num2: the second ciphertext
 */
void check_ee_add_exponent(const EncodedNumber &num1,
                           const EncodedNumber &num2);

#endif // FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_
