//
// Created by wuyuncheng on 30/10/20.
//

#include "falcon/operator/phe/djcs_t_aux.h"

#include <stdio.h>
#include <cstdlib>

#include <glog/logging.h>

void djcs_t_aux_encrypt(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber & res,
    EncodedNumber plain) {
  if (plain.getter_type() != Plaintext) {
    LOG(ERROR) << "The plain should not be encrypted.";
    return;
  }

  mpz_t t1, t2, t3;
  mpz_init(t1);
  mpz_init(t2);
  mpz_init(t3);
  plain.getter_value(t1);
  plain.getter_n(t2);

  djcs_t_encrypt(pk, hr, t3, t1);

  res.setter_n(t2);
  res.setter_value(t3);
  res.setter_exponent(plain.getter_exponent());
  res.setter_type(Ciphertext);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
}

void djcs_t_aux_partial_decrypt(djcs_t_public_key* pk,
    djcs_t_auth_server* au,
    EncodedNumber res,
    EncodedNumber cipher) {
  if (cipher.getter_type() != Ciphertext) {
    LOG(ERROR) << "The value is not ciphertext and cannot be encrypted.";
    return;
  }

  mpz_t t1, t2, t3;
  mpz_init(t1);
  mpz_init(t2);
  mpz_init(t3);
  cipher.getter_n(t1);
  cipher.getter_value(t2);

  res.setter_n(t1);
  res.setter_exponent(cipher.getter_exponent());
  res.setter_type(Plaintext);
  djcs_t_share_decrypt(pk, au, t3, t2);
  res.setter_value(t3);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
}

void djcs_t_aux_share_combine(djcs_t_public_key* pk,
    EncodedNumber res,
    mpz_t* shares) {
  mpz_t t;
  mpz_init(t);
  djcs_t_share_combine(pk, t, shares);
  res.setter_value(t);
  res.setter_type(Plaintext);

  mpz_clear(t);
}

void djcs_t_aux_ee_add(djcs_t_public_key* pk,
    EncodedNumber & res,
    EncodedNumber cipher1,
    EncodedNumber cipher2) {
  if (cipher1.getter_type() != Ciphertext || cipher2.getter_type() != Ciphertext){
    LOG(ERROR) << "The two inputs need be ciphertexts for homomorphic addition.";
    return;
  }

  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);
  cipher1.getter_n(t1);
  cipher2.getter_n(t2);

  if (mpz_cmp(t1, t2) != 0) {
    LOG(ERROR) << "Two ciphertexts are not with the same public key.";
    return;
  }

  if (cipher1.getter_exponent() != cipher2.getter_exponent()) {
    LOG(ERROR) << "Two ciphertexts do not have the same exponents.";
    return;
  }

  mpz_t t3, t4, sum;
  mpz_init(t3);
  mpz_init(t4);
  mpz_init(sum);

  cipher1.getter_value(t3);
  cipher2.getter_value(t4);
  djcs_t_ee_add(pk, sum, t3, t4);
  res.setter_n(t1);
  res.setter_value(sum);
  res.setter_type(Ciphertext);
  res.setter_exponent(cipher1.getter_exponent());

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
  mpz_clear(t4);
  mpz_clear(sum);
}

void djcs_t_aux_ep_mul(djcs_t_public_key* pk,
    EncodedNumber & res,
    EncodedNumber cipher,
    EncodedNumber plain) {
  if (cipher.getter_type() != Ciphertext || plain.getter_type() != Plaintext) {
    LOG(ERROR) << "The input types do not match ciphertext or plaintext.";
    return;
  }

  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);
  cipher.getter_n(t1);
  plain.getter_n(t2);

  if (mpz_cmp(t1, t2) != 0) {
    LOG(ERROR) << "The two inputs are not with the same public key.";
    return;
  }

  mpz_t t3, t4, mult;
  mpz_init(t3);
  mpz_init(t4);
  mpz_init(mult);
  cipher.getter_value(t3);
  plain.getter_value(t4);
  djcs_t_ep_mul(pk, mult, t3, t4);

  res.setter_n(t1);
  res.setter_value(mult);
  res.setter_exponent(cipher.getter_exponent() + plain.getter_exponent());
  res.setter_type(Ciphertext);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
  mpz_clear(t4);
  mpz_clear(mult);
}

void djcs_t_aux_inner_product(djcs_t_public_key* pk,
    hcs_random* hr,
    EncodedNumber& res,
    EncodedNumber* ciphers,
    EncodedNumber* plains,
    int size) {
  if (size == 0) {
    LOG(ERROR) << "The size of two vectors is zero.";
    return;
  }

  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);
  ciphers[0].getter_n(t1);
  plains[0].getter_n(t2);

  if (mpz_cmp(t1, t2) != 0) {
    LOG(ERROR) << "The two vectors are not with the same public key.";
    return;
  }

  // assume the elements in the plains have the same exponent, so does ciphers
  res.setter_n(t1);
  res.setter_exponent(ciphers[0].getter_exponent() + plains[0].getter_exponent());
  res.setter_type(Ciphertext);

  mpz_t *mpz_ciphers = (mpz_t *) malloc (size * sizeof(mpz_t));
  mpz_t *mpz_plains = (mpz_t *) malloc (size * sizeof(mpz_t));
  for (int i = 0; i < size; i++) {
    mpz_init(mpz_ciphers[i]);
    mpz_init(mpz_plains[i]);
    ciphers[i].getter_value(mpz_ciphers[i]);
    plains[i].getter_value(mpz_plains[i]);
  }

  // homomorphic dot product
  mpz_t sum, t3;   // why need another sum1 that can only be correct? weired
  mpz_init(sum);
  mpz_init(t3);
  mpz_set_si(t3, 0);
  djcs_t_encrypt(pk, hr, sum, t3);

  for (int j = 0; j < size; j++) {
    // store the encrypted_exact_answer
    mpz_t tmp;
    mpz_init(tmp);
    djcs_t_ep_mul(pk, tmp, mpz_ciphers[j], mpz_plains[j]);
    djcs_t_ee_add(pk, sum, sum, tmp);

    mpz_clear(tmp);
  }

  res.setter_value(sum);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
  mpz_clear(sum);

  for (int i = 0; i < size; i++) {
    mpz_clear(mpz_ciphers[i]);
    mpz_clear(mpz_plains[i]);
  }
  free(mpz_ciphers);
  free(mpz_plains);
}