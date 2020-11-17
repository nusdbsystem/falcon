//
// Created by wuyuncheng on 30/10/20.
//

#ifndef FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_
#define FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_

#include <vector>

#include "gmp.h"
#include "libhcs.h"

#include "falcon/operator/phe/fixed_point_encoder.h"

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
    EncodedNumber plain);

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
    EncodedNumber cipher);

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
    EncodedNumber cipher1,
    EncodedNumber cipher2);

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
    EncodedNumber cipher,
    EncodedNumber plain);

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
void djcs_t_aux_matrix_mult(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber* res,
    EncodedNumber* ciphers,
    EncodedNumber** plains,
    int row_size,
    int column_size);

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

#endif //FALCON_SRC_OPERATOR_PHE_DJCS_T_AUX_H_
