//
// Created by wuyuncheng on 16/8/20.
//

#include "fixed_point_encoder.h"

#include <iomanip>
#include <sstream>
#include <cmath>
#include <iostream>

#include <glog/logging.h>

EncodedNumber::EncodedNumber()
{
  mpz_init(n);
  mpz_init(value);
  type = Plaintext;
}

EncodedNumber::EncodedNumber(const EncodedNumber &number)
{
  mpz_init(n);
  mpz_init(value);

  mpz_t n_helper, value_helper;
  mpz_init(n_helper);
  mpz_init(value_helper);
  number.getter_n(n_helper);
  number.getter_value(value_helper);

  mpz_set(n, n_helper);
  mpz_set(value, value_helper);
  exponent = number.getter_exponent();
  type = number.getter_type();

  mpz_clear(n_helper);
  mpz_clear(value_helper);
}

EncodedNumber& EncodedNumber::operator=(const EncodedNumber &number) {
  mpz_init(n);
  mpz_init(value);

  mpz_t n_helper, value_helper;
  mpz_init(n_helper);
  mpz_init(value_helper);
  number.getter_n(n_helper);
  number.getter_value(value_helper);

  mpz_set(n, n_helper);
  mpz_set(value, value_helper);
  exponent = number.getter_exponent();
  type = number.getter_type();

  mpz_clear(n_helper);
  mpz_clear(value_helper);
}

EncodedNumber::~EncodedNumber()
{
  //gmp_printf("value = %Zd\n", value);
  mpz_clear(n);
  mpz_clear(value);
}

void EncodedNumber::setter_n(mpz_t s_n) {
  mpz_set(n, s_n);
}

void EncodedNumber::setter_exponent(int s_exponent) {
  exponent = s_exponent;
}

void EncodedNumber::setter_value(mpz_t s_value) {
  mpz_set(value, s_value);
}

void EncodedNumber::setter_type(EncodedNumberType s_type) {
  type = s_type;
}

int EncodedNumber::getter_exponent() const {
  return exponent;
}

void EncodedNumber::getter_n(__mpz_struct *g_n) const {
  mpz_set(g_n, n);
}

void EncodedNumber::getter_value(__mpz_struct *g_value) const {
  mpz_set(g_value, value);
}

EncodedNumberType EncodedNumber::getter_type() const {
  return type;
}