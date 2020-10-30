//
// Created by wuyuncheng on 16/8/20.
//

#include "falcon/operator/phe/fixed_point_encoder.h"

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

void EncodedNumber::set_integer(mpz_t pn, int v) {
  mpz_set(n, pn);
  fixed_pointed_encode(v, value, exponent);
}

void EncodedNumber::set_float(mpz_t pn, float v, int precision) {
  mpz_set(n, pn);
  fixed_pointed_encode(v, precision, value, exponent);
}

void EncodedNumber::decrease_exponent(int new_exponent)
{
  if (new_exponent > exponent) {
    LOG(ERROR) << "New exponent should be more negative than old exponent.";
    return;
  }

  if (new_exponent == exponent) return;

  unsigned long int exp_diff = exponent - new_exponent;
  mpz_t t;
  mpz_init(t);
  mpz_ui_pow_ui(t, PHE_FIXED_POINT_BASE, exp_diff);
  mpz_mul(value, value, t);
  exponent = new_exponent;
}

void EncodedNumber::increase_exponent(int new_exponent)
{
  if (new_exponent < exponent) {
    LOG(ERROR) << "New exponent should be more positive than old exponent.";
    return;
  }

  if (new_exponent == exponent) return;

  mpz_t t, new_value;
  mpz_init(t);
  mpz_init(new_value);
  unsigned long int exp_diff = new_exponent - exponent;
  mpz_ui_pow_ui(t, PHE_FIXED_POINT_BASE, exp_diff);
  mpz_cdiv_q(new_value, value, t);

  exponent = new_exponent;
  mpz_set(value, new_value);

  mpz_clear(t);
  mpz_clear(new_value);
}

void EncodedNumber::decode(long &v)
{
  if (exponent != 0) {
    // not integer, should not call this decode function
    LOG(ERROR) << "Exponent is not zero, failed, should call decode with float.";
    return;
  }

  switch (check_encoded_number()) {
    case Positive:
      fixed_pointed_decode(v, value);
      break;
    case Negative:
      mpz_sub(value, value, n);
      fixed_pointed_decode(v, value);
      break;
    case Overflow:
      LOG(ERROR) << "Encoded number is overflow.";
      break;
    default:
      LOG(ERROR) << "Encoded number is corrupted.";
      return;
  }
}

void EncodedNumber::decode(float &v)
{
  switch (check_encoded_number()) {
    case Positive:
      fixed_pointed_decode(v, value, exponent);
      break;
    case Negative:
      mpz_sub(value, value, n);
      fixed_pointed_decode(v, value, exponent);
      break;
    case Overflow:
      LOG(ERROR) << "Encoded number is overflow.";
      break;
    default:
      LOG(ERROR) << "Encoded number is corrupted.";
      return;
  }
}

void EncodedNumber::decode_with_truncation(float &v, int truncated_exponent)
{
  switch (check_encoded_number()) {
    case Positive:
      fixed_pointed_decode_truncated(v, value, exponent, truncated_exponent);
      break;
    case Negative:
      mpz_sub(value, value, n);
      fixed_pointed_decode_truncated(v, value, exponent, truncated_exponent);
      break;
    case Overflow:
      LOG(ERROR) << "Encoded number is overflow.";
      break;
    default:
      LOG(ERROR) << "Encoded number is corrupted.";
      return;
  }
}

EncodedNumberState EncodedNumber::check_encoded_number()
{
  // max_int is the threshold of positive number
  // neg_int is the threshold of negative number
  mpz_t max_int, neg_int;
  mpz_init(max_int);
  compute_decode_threshold(max_int);
  mpz_init(neg_int);
  mpz_sub(neg_int, n, max_int);

  // gmp_printf("max_int is %Zd\n", max_int);
  // gmp_printf("neg_int is %Zd\n", neg_int);

  EncodedNumberState state;
  if (mpz_cmp(value, n) >= 0) {
    state = Invalid;
  } else if (mpz_cmp(value, max_int) <= 0) {
    state = Positive;
  } else if (mpz_cmp(value, neg_int) >= 0) {
    state = Negative;
  } else {
    state = Overflow;
  }

  // printf("state is %d\n", state);

  mpz_clear(max_int);
  mpz_clear(neg_int);
  return state;
}

void EncodedNumber::compute_decode_threshold(mpz_t max_int)
{
  mpz_t t;
  mpz_init(t);
  mpz_fdiv_q_ui(t, n, 3);
  mpz_sub_ui(max_int, t, 1);  // this is max int
  mpz_clear(t);
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

// helper functions bellow
long long fixed_pointed_integer_representation(float value, int precision){
  long long ex = (long long) pow(PHE_FIXED_POINT_BASE, precision);
  std::stringstream ss;
  ss << std::fixed << std::setprecision(precision) << value;
  std::string s = ss.str();
  long long r = (long long) (::atof(s.c_str()) * ex);
  return r;
}

void fixed_pointed_encode(long value, mpz_t res, int & exponent) {
  mpz_set_si(res, value);
  exponent = 0;
}

void fixed_pointed_encode(float value, int precision, mpz_t res, int & exponent) {
  long long r = fixed_pointed_integer_representation(value, precision);
  mpz_set_si(res, r);
  exponent = 0 - precision;
}

void fixed_pointed_decode(long & value, mpz_t res) {
  value = mpz_get_si(res);
}

void fixed_pointed_decode(float & value, mpz_t res, int exponent) {
  if (exponent >= 0) {
    LOG(ERROR) << "Decode mpz_t for float value failed.";
    return;
  }
  if (exponent < 0 - PHE_MAXIMUM_FIXED_POINT_PRECISION) {
    fixed_pointed_decode_truncated(value, res, exponent, 0 - PHE_MAXIMUM_FIXED_POINT_PRECISION);
  } else {
    char *t = mpz_get_str(NULL, 10, res);
    long v = ::atol(t);

    if (v == 0) {
      value = 0;
    } else {
      value = (float) (v * pow(PHE_FIXED_POINT_BASE, exponent));
    }
    free(t);
  }
}

void fixed_pointed_decode_truncated(float & value, mpz_t res, int exponent, int truncated_exponent) {
  if (exponent >= 0 || truncated_exponent >= 0) {
    LOG(ERROR) << "Decode mpz_t for float value failed.";
    return;
  }

  if (exponent >= truncated_exponent) {
    char *t = mpz_get_str(NULL, 10, res);
    long v = ::atol(t);

    if (v == 0) {
      value = 0;
    } else {
      value = (float) (v * pow(PHE_FIXED_POINT_BASE, exponent));
      // printf("decoded value = %f\n", value);
    }
    free(t);
  } else {
    // convert res to truncated_exponent
    unsigned long int exp_diff = truncated_exponent - exponent;
    // printf("exp_diff = %ld\n", exp_diff);
    mpz_t tmp, new_value;
    mpz_init(tmp);
    mpz_init(new_value);
    mpz_ui_pow_ui(tmp, PHE_FIXED_POINT_BASE, exp_diff);
    mpz_cdiv_q(new_value, res, tmp);
    // gmp_printf("tmp = %Zd\n", tmp);
    // gmp_printf("new_value = %Zd\n", new_value);

    // decode with new_value
    char *t_new = mpz_get_str(NULL, 10, new_value);
    long v_new = ::atol(t_new);

    // printf("v_new = %ld\n", v_new);

    if (v_new == 0) {
      value = 0;
    } else {
      value = (float) (v_new * pow(PHE_FIXED_POINT_BASE, truncated_exponent));
    }
    free(t_new);
    mpz_clear(tmp);
    mpz_clear(new_value);
  }
}