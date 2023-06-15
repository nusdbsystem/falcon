//
// Created by root on 3/11/23.
//

/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string>
#include <ctime>

#include "falcon/operator/phe/djcs_t_aux.h"
#include <gtest/gtest.h>
#include <random>

using namespace std;

double f_rand(double f_min, double f_max)
{
  std::uniform_real_distribution<double> unif(f_min, f_max);
  std::default_random_engine re;
  double a_random_double = unif(re);
  return a_random_double;
}

TEST(PHE, DJCS_BENCH) {
  // benchmark encryption, decryption, addition, and multiplication time
  std::vector<int> key_sizes{256, 512, 1024, 2048, 3072};
  int test_size = 1000;
  int client_num = 3;
  double f_min = 0.0;
  double f_max = 10000.0;
  int precision = 32;

  for (int key_size : key_sizes) {
    std::cout << "Benchmarking key_size = " << key_size << std::endl;
    // generate keys and init djcs_t parameters
    hcs_random *hr = hcs_init_random();
    djcs_t_public_key *pk = djcs_t_init_public_key();
    djcs_t_private_key *vk = djcs_t_init_private_key();
    auto **au =
        (djcs_t_auth_server **)malloc(client_num * sizeof(djcs_t_auth_server *));
    auto *si = (mpz_t *)malloc(client_num * sizeof(mpz_t));
    // generate key pair
    djcs_t_generate_key_pair(pk, vk, hr, 1, key_size, client_num, client_num);
    mpz_t *coeff = djcs_t_init_polynomial(vk, hr);
    for (int i = 0; i < client_num; i++) {
      mpz_init(si[i]);
      djcs_t_compute_polynomial(vk, coeff, si[i], i);
      au[i] = djcs_t_init_auth_server();
      djcs_t_set_auth_server(au[i], si[i], i);
    }

    // init vectors
    auto* plain_test_vec1 = new EncodedNumber[test_size];
    auto* plain_test_vec2 = new EncodedNumber[test_size];
    auto* enc_test_vec1 = new EncodedNumber[test_size];
    auto* enc_test_vec2 = new EncodedNumber[test_size];
    for (int i = 0; i < test_size; i++) {
      double d1 = f_rand(f_min, f_max);
      double d2 = f_rand(f_min, f_max);
      plain_test_vec1[i].set_double(pk->n[0], d1, precision);
      plain_test_vec2[i].set_double(pk->n[0], d2, precision);
      djcs_t_aux_encrypt(pk, hr, enc_test_vec1[i], plain_test_vec1[i]);
      djcs_t_aux_encrypt(pk, hr, enc_test_vec2[i], plain_test_vec2[i]);
    }

    struct timespec encryption_start_time{}, addition_start_time{}, multiplication_start_time{};
    struct timespec encryption_finish_time{}, addition_finish_time{}, multiplication_finish_time{};

    std::cout << "Initialization finished." << std::endl;

    // encryption benchmark
    clock_gettime(CLOCK_MONOTONIC, &encryption_start_time);
    auto* encryption_help_vec = new EncodedNumber[test_size];
    for (int i = 0; i < test_size; i++) {
      djcs_t_aux_encrypt(pk, hr, encryption_help_vec[i], plain_test_vec1[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &encryption_finish_time);
    auto encryption_consumed_time =
        (double)(encryption_finish_time.tv_sec - encryption_start_time.tv_sec);
     encryption_consumed_time +=
        (double)(encryption_finish_time.tv_nsec - encryption_start_time.tv_nsec) /
            1000000000.0;
    std::cout << "The average encryption time of " << test_size
      << " operations is " << (encryption_consumed_time / test_size) << std::endl;
    delete [] encryption_help_vec;

    // homomorphic addition benchmark
    clock_gettime(CLOCK_MONOTONIC, &addition_start_time);
    auto* addition_help_vec = new EncodedNumber[test_size];
    for (int i = 0; i < test_size; i++) {
      djcs_t_aux_ee_add(pk, addition_help_vec[i], enc_test_vec1[i], enc_test_vec2[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &addition_finish_time);
    auto addition_consumed_time =
        (double)(addition_finish_time.tv_sec - addition_start_time.tv_sec);
     addition_consumed_time +=
        (double)(addition_finish_time.tv_nsec - addition_start_time.tv_nsec) /
            1000000000.0;
    std::cout << "The average addition time of " << test_size
              << " operations is " << (addition_consumed_time / test_size) << std::endl;
    delete [] addition_help_vec;

    // homomorphic multiplication benchmark
    clock_gettime(CLOCK_MONOTONIC, &multiplication_start_time);
    auto* multiplication_help_vec = new EncodedNumber[test_size];
    for (int i = 0; i < test_size; i++) {
      djcs_t_aux_ep_mul(pk, multiplication_help_vec[i], enc_test_vec1[i], plain_test_vec2[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &multiplication_finish_time);
    auto multiplication_consumed_time =
        (double)(multiplication_finish_time.tv_sec - multiplication_start_time.tv_sec);
     multiplication_consumed_time +=
        (double)(multiplication_finish_time.tv_nsec - multiplication_start_time.tv_nsec) /
            1000000000.0;
    std::cout << "The average multiplication time of " << test_size
              << " operations is " << (multiplication_consumed_time / test_size) << std::endl;
    delete [] multiplication_help_vec;

    delete [] plain_test_vec1;
    delete [] plain_test_vec2;
    delete [] enc_test_vec1;
    delete [] enc_test_vec2;

    // free key related parameters
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
}
