//
// Created by wuyuncheng on 30/10/20.
//

#ifndef FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_
#define FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_

#include <vector>

#include "gmp.h"    // gmp is included implicitly
#include "libhcs.h" // master header includes everything

#include "falcon/operator/phe/fixed_point_encoder.h"

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
void djcs_t_aux_encrypt(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber & res,
    const EncodedNumber& plain);

/**
 * decrypt a ciphertext EncodedNumber
 *
 * @param pk: public key
 * @param au: decryption authenticated server
 * @param res: partially decrypted EncodedNumber
 * @param cipher: ciphertext EncodedNumber
 */
void djcs_t_aux_partial_decrypt(djcs_t_public_key* pk,
    djcs_t_auth_server* au,
    EncodedNumber & res,
    const EncodedNumber& cipher);

/**
 * combine partially decrypted EncodedNumbers
 *
 * @param pk: public key
 * @param res: decrypted EncodedNumber
 * @param shares: partially decrypted EncodedNumbers
 * @param size: size of the shares vector
 */
void djcs_t_aux_share_combine(djcs_t_public_key* pk,
    EncodedNumber & res,
    EncodedNumber* shares,
    int size);

/**
 * homomorphic addition of two ciphers and return an EncodedNumber
 *
 * @param pk: public key
 * @param res: summation ciphertext
 * @param cipher1: first ciphertext EncodedNumber
 * @param cipher2: second ciphertext EncodedNumber
 */
void djcs_t_aux_ee_add(djcs_t_public_key* pk,
    EncodedNumber & res,
    const EncodedNumber& cipher1,
    const EncodedNumber& cipher2);

/**
 * homomorphic multiplication of a cipher and a plain
 * return an EncodedNumber
 *
 * @param pk: public key
 * @param res: multiplication ciphertext
 * @param cipher: ciphertext EncodedNumber
 * @param plain: plaintext EncodedNumber
 */
void djcs_t_aux_ep_mul(djcs_t_public_key* pk,
    EncodedNumber & res,
    const EncodedNumber& cipher,
    const EncodedNumber& plain);


/***********************************************************/
/***************** vector related operations ***************/
/***********************************************************/

/**
 * homomorphic aggregate a cipher vector and return the result
 *
 * @param pk: public key
 * @param res: aggregated ciphertext
 * @param ciphers: a vector of ciphertext EncodedNumber
 * @param size: the size of the ciphers vector
 */
void djcs_t_aux_vec_aggregate(djcs_t_public_key* pk,
                              EncodedNumber& res,
                              EncodedNumber* ciphers,
                              int size);

/**
 * element-wise homomorphic ciphertext add
 *
 * @param pk: public key
 * @param res: the resulted cipher vector
 * @param ciphers1: the cipher vector 1
 * @param ciphers2: the cipher vector 2
 * @param size: the size of the two cipher vectors
 */
void djcs_t_aux_vec_ele_wise_ee_add(
    djcs_t_public_key* pk,
    EncodedNumber* res,
    EncodedNumber* ciphers1,
    EncodedNumber* ciphers2,
    int size);

/**
 * homomorphic inner product of a cipher vector and a plain vector
 * return an EncodedNumber
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: inner product ciphertext EncodedNumber
 * @param ciphers: a vector of ciphertext EncodedNumber
 * @param plains: a vector of plaintext EncodedNumber
 * @param size: vector size
 */
void djcs_t_aux_inner_product(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber& res,
    EncodedNumber* ciphers,
    EncodedNumber* plains,
    int size);

/**
 * element-wise ciphertext plaintext multiplication
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: resulted ciphertext vector
 * @param ciphers: a vector of ciphertext EncodedNumber
 * @param plains: a vector of plaintext EncodedNumber
 * @param size: vector size
 */
void djcs_t_aux_vec_ele_wise_ep_mul(djcs_t_public_key* pk,
    EncodedNumber* res,
    EncodedNumber* ciphers,
    EncodedNumber* plains,
    int size);

/***********************************************************/
/***************** matrix related operations ***************/
/***********************************************************/

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
void djcs_t_aux_matrix_ele_wise_ee_add(
    djcs_t_public_key* pk,
    EncodedNumber** res,
    EncodedNumber** cipher_mat1,
    EncodedNumber** cipher_mat2,
    int row_size,
    int column_size);

/**
 * homomorphic matrix multiplication of a cipher vector
 * and a plain matrix, the result is a cipher vector
 *
 * @param pk: public key
 * @param hr: random variable
 * @param res: multiplication results with row_size
 * @param ciphers: cipher vector
 * @param plains: plain matrix
 * @param row_size: number of plaintext rows
 * @param column_size: number of columns, equal to cipher size
 */
void djcs_t_aux_vec_mat_ep_mult(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber* res,
    EncodedNumber* ciphers,
    EncodedNumber** plains,
    int row_size,
    int column_size);

/**
 * the homomorphic multiplication between a plaintext matrix
 * and a ciphertext matrix, need to ensure plain_column_size = cipher_row_size
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
void djcs_t_aux_mat_mat_ep_mult(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber** res,
    EncodedNumber** cipher_mat,
    EncodedNumber** plain_mat,
    int cipher_row_size,
    int cipher_column_size,
    int plain_row_size,
    int plain_column_size);

/**
 * copy a djcs_t public key from src to dest
 *
 * @param src: src public key object
 * @param dest: dest public key object
 */
void djcs_t_public_key_copy(djcs_t_public_key* src, djcs_t_public_key* dest);

/**
 * copy a djcs_t authenticate server from src to dest
 *
 * @param src: src auth server object
 * @param dest: dest auth server object
 */
void djcs_t_auth_server_copy(djcs_t_auth_server* src, djcs_t_auth_server* dest);

/**
 * copy a hcs_random from src to dest
 *
 * @param src: src hcs random object
 * @param dest: dest hcs random object
 */
void djcs_t_hcs_random_copy(hcs_random* src, hcs_random* dest);

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
void check_encoded_public_key(const EncodedNumber& num1, const EncodedNumber& num2);

/**
 * check the two ciphertexts have the same exponent
 *
 * @param num1: the first ciphertext
 * @param num2: the second ciphertext
 */
void check_ee_add_exponent(const EncodedNumber& num1, const EncodedNumber& num2);

#endif //FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_
