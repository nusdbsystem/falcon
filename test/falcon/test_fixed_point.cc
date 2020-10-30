//
// Created by wuyuncheng on 13/9/20.
//

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#include "falcon/operator/phe/fixed_point_encoder.h"

using namespace std;

TEST(FixedPoint, SetterGetter) {
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "100", 10);
  int v_exponent = -8;
  EncodedNumberType v_type = Plaintext;

  EncodedNumber number;
  number.setter_n(v_n);
  number.setter_value(v_value);
  number.setter_exponent(v_exponent);
  number.setter_type(v_type);

  mpz_t g_n;
  mpz_t g_value;
  mpz_init(g_n);
  mpz_init(g_value);
  number.getter_n(g_n);
  number.getter_value(g_value);
  int g_exponent = number.getter_exponent();
  EncodedNumberType g_type = number.getter_type();
  // gmp_printf("g_n = %Zd\n", g_n);
  // gmp_printf("g_value = %Zd\n", g_value);

  int n_cmp = mpz_cmp(v_n, g_n);
  int value_cmp = mpz_cmp(v_value, g_value);
  // printf("n_cmp = %d\n", n_cmp);
  // printf("value_cmp = %d\n", value_cmp);

  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(g_exponent, v_exponent);
  EXPECT_EQ(g_type, v_type);

  mpz_clear(v_n);
  mpz_clear(v_value);
  mpz_clear(g_n);
  mpz_clear(g_value);
}

TEST(FixedPoint, IntegerEncodeDecode){
  // positive integer
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "5", 10);

  int positive = 5;
  EncodedNumber positive_number;
  positive_number.set_integer(v_n, positive);
  mpz_t g_n;
  mpz_t g_value;
  mpz_init(g_n);
  mpz_init(g_value);
  positive_number.getter_n(g_n);
  positive_number.getter_value(g_value);

  int n_cmp = mpz_cmp(v_n, g_n);
  int value_cmp = mpz_cmp(v_value, g_value);

  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(0, positive_number.getter_exponent());
  EXPECT_EQ(Plaintext, positive_number.getter_type());

  long decoded_positive;
  positive_number.decode(decoded_positive);
  // printf("decoded_positive = %ld\n", decoded_positive);
  EXPECT_EQ((long) positive, decoded_positive);

  // negative integer
  int negative = -8000;
  mpz_init(v_value);
  mpz_set_str(v_value, "-8000", 10);

  EncodedNumber negative_number;
  negative_number.set_integer(v_n, negative);
  mpz_clear(g_n);
  mpz_clear(g_value);
  mpz_init(g_n);
  mpz_init(g_value);
  negative_number.getter_n(g_n);
  negative_number.getter_value(g_value);

  n_cmp = mpz_cmp(v_n, g_n);
  value_cmp = mpz_cmp(v_value, g_value);

  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(0, negative_number.getter_exponent());
  EXPECT_EQ(Plaintext, negative_number.getter_type());

  long decoded_negative;
  negative_number.decode(decoded_negative);
  // printf("decoded_negative = %ld\n", decoded_negative);
  EXPECT_EQ((long) negative, decoded_negative);

  mpz_clear(v_n);
  mpz_clear(v_value);
  mpz_clear(g_n);
  mpz_clear(g_value);
}

TEST(FixedPoint, FloatEncodeDecode){
  // positive float
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "8090", 10);

  float positive = 0.123456;
  EncodedNumber positive_number;
  positive_number.set_float(v_n, positive, 16);
  mpz_t g_n;
  mpz_t g_value;
  mpz_init(g_n);
  mpz_init(g_value);
  positive_number.getter_n(g_n);
  positive_number.getter_value(g_value);
  // gmp_printf("positive_number = %Zd\n", g_value);

  int n_cmp = mpz_cmp(v_n, g_n);
  int value_cmp = mpz_cmp(v_value, g_value);

  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(-16, positive_number.getter_exponent());
  EXPECT_EQ(Plaintext, positive_number.getter_type());

  float decoded_positive;
  positive_number.decode(decoded_positive);
  // printf("decoded_positive = %ld\n", decoded_positive);
  EXPECT_NEAR(positive, decoded_positive, 1e-3);

  // negative float
  float negative = -0.666666;
  mpz_init(v_value);
  mpz_set_str(v_value, "-43690", 10);

  EncodedNumber negative_number;
  negative_number.set_float(v_n, negative, 16);
  mpz_clear(g_n);
  mpz_clear(g_value);
  mpz_init(g_n);
  mpz_init(g_value);
  negative_number.getter_n(g_n);
  negative_number.getter_value(g_value);

  n_cmp = mpz_cmp(v_n, g_n);
  value_cmp = mpz_cmp(v_value, g_value);

  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(-16, negative_number.getter_exponent());
  EXPECT_EQ(Plaintext, negative_number.getter_type());

  float decoded_negative;
  negative_number.decode(decoded_negative);
  // printf("decoded_negative = %ld\n", decoded_negative);
  EXPECT_NEAR(negative, decoded_negative, 1e-3);

  mpz_clear(v_n);
  mpz_clear(v_value);
  mpz_clear(g_n);
  mpz_clear(g_value);
}

TEST(FixedPoint, EncodedState){
  mpz_t v_n;
  mpz_t v_value_invalid, v_value_positive, v_value_negative, v_value_overflow;
  mpz_init(v_n);
  mpz_init(v_value_invalid);
  mpz_init(v_value_positive);
  mpz_init(v_value_negative);
  mpz_init(v_value_overflow);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value_invalid, "100000000000000", 10);
  mpz_set_str(v_value_positive, "33333333333332", 10);
  mpz_set_str(v_value_negative, "66666666666668", 10);
  mpz_set_str(v_value_overflow, "66666666666667", 10);
  int v_exponent = 0;
  EncodedNumberType v_type = Plaintext;

  mpz_t max_int, neg_int;
  mpz_init(max_int);
  mpz_init(neg_int);

  EncodedNumber number;
  number.setter_n(v_n);
  number.setter_value(v_value_invalid);
  number.setter_exponent(v_exponent);
  number.setter_type(v_type);

  number.compute_decode_threshold(max_int);
  int cmp = mpz_cmp(max_int, v_value_positive);
  EXPECT_EQ(0, cmp);
  mpz_sub(neg_int, v_n, max_int);
  cmp = mpz_cmp(neg_int, v_value_negative);
  EXPECT_EQ(0, cmp);

  EncodedNumberState state;
  state = number.check_encoded_number();
  EXPECT_EQ(Invalid, state);

  number.setter_value(v_value_positive);
  state = number.check_encoded_number();
  EXPECT_EQ(Positive, state);

  number.setter_value(v_value_negative);
  state = number.check_encoded_number();
  EXPECT_EQ(Negative, state);

  number.setter_value(v_value_overflow);
  state = number.check_encoded_number();
  EXPECT_EQ(Overflow, state);

  mpz_clear(v_n);
  mpz_clear(v_value_invalid);
  mpz_clear(v_value_positive);
  mpz_clear(v_value_negative);
  mpz_clear(v_value_overflow);
  mpz_clear(max_int);
  mpz_clear(neg_int);
}

TEST(FixedPoint, ExponentChange){
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "100", 10);
  //int v_exponent = 0 - PHE_FIXED_POINT_PRECISION;
  int v_exponent = -8;
  EncodedNumberType v_type = Plaintext;

  EncodedNumber number;
  number.setter_n(v_n);
  number.setter_value(v_value);
  number.setter_exponent(v_exponent);
  number.setter_type(v_type);

  // decrease exponent
  number.decrease_exponent(-16);
  mpz_t v_value_change, g_value;
  mpz_init(v_value_change);
  mpz_init(g_value);
  mpz_set_str(v_value_change, "25600", 10);
  number.getter_value(g_value);
  // gmp_printf("g_value = %Zd\n", g_value);
  int cmp = mpz_cmp(v_value_change, g_value);
  int exponent = number.getter_exponent();
  EXPECT_EQ(0, cmp);
  EXPECT_EQ(exponent, -16);

  // increase exponent
  number.increase_exponent(-8);
  mpz_clear(g_value);
  mpz_init(g_value);
  number.getter_value(g_value);
  // gmp_printf("g_value = %Zd\n", g_value);
  cmp = mpz_cmp(v_value, g_value);
  EXPECT_EQ(0, cmp);
  exponent = number.getter_exponent();
  EXPECT_EQ(exponent, -8);

  mpz_clear(v_n);
  mpz_clear(v_value);
  mpz_clear(v_value_change);
  mpz_clear(g_value);
}

TEST(FixedPoint, DecodeTruncation){
  mpz_t v_n;
  mpz_init(v_n);
  mpz_set_str(v_n, "100000000000000", 10);

  EncodedNumber number;
  number.set_float(v_n, -0.00105, 32);
  float x_decoded, x_decoded_truncation;
  number.decode(x_decoded);
  number.decode_with_truncation(x_decoded_truncation, -16);
  EXPECT_NEAR(x_decoded, x_decoded_truncation, 1e-3);
  // printf("x_decoded = %f\n", x_decoded);
  // printf("x_decoded_truncation = %f\n", x_decoded_truncation);

  mpz_clear(v_n);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}