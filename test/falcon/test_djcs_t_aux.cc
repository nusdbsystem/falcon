//
// Created by wuyuncheng on 30/10/20.
//

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#include "falcon/operator/phe/djcs_t_aux.h"

using namespace std;

TEST(PHE, ThresholdPaillierScheme) {
  // init djcs_t parameters
  int client_num = 3;
  hcs_random *hr = hcs_init_random();
  djcs_t_public_key *pk = djcs_t_init_public_key();
  djcs_t_private_key *vk = djcs_t_init_private_key();
  djcs_t_auth_server **au = (djcs_t_auth_server **)malloc(client_num * sizeof(djcs_t_auth_server *));
  mpz_t *si = (mpz_t *)malloc(client_num * sizeof(mpz_t));

  // generate key pair
  djcs_t_generate_key_pair(pk, vk, hr, 1, 1024, client_num, client_num);
  hr = hcs_init_random();
  mpz_t *coeff = djcs_t_init_polynomial(vk, hr);
  for (int i = 0; i < client_num; i++) {
    mpz_init(si[i]);
    djcs_t_compute_polynomial(vk, coeff, si[i], i);
    au[i] = djcs_t_init_auth_server();
    djcs_t_set_auth_server(au[i], si[i], i);
  }

  //////////////////////////////////////
  /// Test encryption and decryption ///
  //////////////////////////////////////
  float original_number = -0.666;
  EncodedNumber number;
  number.set_float(pk->n[0], original_number, 16);

  EncodedNumber encrypted_number, decrypted_number;
  djcs_t_aux_encrypt(pk, hr, encrypted_number, number);

  EncodedNumber* partially_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_decryption[i], encrypted_number);
  }
  djcs_t_aux_share_combine(pk, decrypted_number, partially_decryption, client_num);

  float decrypted_decoded_number;
  decrypted_number.decode(decrypted_decoded_number);
  EXPECT_NEAR(original_number, decrypted_decoded_number, 1e-3);

  //////////////////////////////////////
  ///// Test homomorphic addition //////
  //////////////////////////////////////
  float a1 = 5.25;
  float a2 = -0.38;
  EncodedNumber number_a1, number_a2;
  number_a1.set_float(pk->n[0], a1, 16);
  number_a2.set_float(pk->n[0], a2, 16);

  EncodedNumber encrypted_number_a1, encrypted_number_a2;
  djcs_t_aux_encrypt(pk, hr, encrypted_number_a1, number_a1);
  djcs_t_aux_encrypt(pk, hr, encrypted_number_a2, number_a2);

  EncodedNumber encrypted_sum, decrypted_sum;
  djcs_t_aux_ee_add(pk, encrypted_sum, encrypted_number_a1, encrypted_number_a2);

  EncodedNumber* partially_sum_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_sum_decryption[i], encrypted_sum);
  }
  djcs_t_aux_share_combine(pk, decrypted_sum, partially_sum_decryption, client_num);

  float decrypted_decoded_sum;
  decrypted_sum.decode(decrypted_decoded_sum);
  EXPECT_NEAR(a1 + a2, decrypted_decoded_sum, 1e-3);

  //////////////////////////////////////
  /// Test homomorphic multiplication //
  //////////////////////////////////////
  float b1 = 5.55;
  float b2 = -0.58;
  EncodedNumber number_b1, number_b2;
  number_b1.set_float(pk->n[0], b1, 16);
  number_b2.set_float(pk->n[0], b2, 16);

  EncodedNumber encrypted_number_b1;
  djcs_t_aux_encrypt(pk, hr, encrypted_number_b1, number_b1);

  EncodedNumber encrypted_product, decrypted_product;
  djcs_t_aux_ep_mul(pk, encrypted_product, encrypted_number_b1, number_b2);

  EncodedNumber* partially_product_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_product_decryption[i], encrypted_product);
  }
  djcs_t_aux_share_combine(pk, decrypted_product, partially_product_decryption, client_num);

  float decrypted_decoded_product;
  decrypted_product.decode(decrypted_decoded_product);
  EXPECT_NEAR(b1 * b2, decrypted_decoded_product, 1e-3);

  //////////////////////////////////////
  /// Test homomorphic inner product ///
  //////////////////////////////////////
  float c1[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  float c2[5] = {0.9, 0.8, -0.7, 0.6, 0.5};
  EncodedNumber* number_c1 = new EncodedNumber[5];
  EncodedNumber* number_c2 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_c1[i].set_float(pk->n[0], c1[i], 16);
    number_c2[i].set_float(pk->n[0], c2[i], 16);
  }
  EncodedNumber* encrypted_number_c1 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, encrypted_number_c1[i], number_c1[i]);
  }

  EncodedNumber encrypted_inner_product, decrypted_inner_product;
  djcs_t_aux_inner_product(pk, hr, encrypted_inner_product, encrypted_number_c1, number_c2, 5);

  EncodedNumber* partially_inner_prod_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_inner_prod_decryption[i], encrypted_inner_product);
  }
  djcs_t_aux_share_combine(pk, decrypted_inner_product, partially_inner_prod_decryption, client_num);

  float decrypted_decoded_inner_product;
  decrypted_inner_product.decode(decrypted_decoded_inner_product);
  float inner_product = 0.0;
  for (int i = 0; i < 5; i++) {
    inner_product = inner_product + c1[i] * c2[i];
  }
  EXPECT_NEAR(inner_product, decrypted_decoded_inner_product, 1e-3);
  //printf("inner_product = %f\n", inner_product);
  //printf("decrypted_decoded_inner_product = %f\n", decrypted_decoded_inner_product);

  // free memory
  delete [] partially_decryption;
  delete [] partially_sum_decryption;
  delete [] partially_product_decryption;
  delete [] partially_inner_prod_decryption;
  delete [] number_c1;
  delete [] number_c2;
  delete [] encrypted_number_c1;

  hcs_free_random(hr);
  djcs_t_free_public_key(pk);
  djcs_t_free_private_key(vk);

  for (int i = 0; i < client_num; i++) {
    mpz_clear(si[i]);
    djcs_t_free_auth_server(au[i]);
  }
  djcs_t_free_polynomial(vk, coeff);

  free(si);
  free(au);
}