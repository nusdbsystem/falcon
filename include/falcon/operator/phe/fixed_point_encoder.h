//
// Created by wuyuncheng on 16/8/20.
//

#ifndef FALCON_SRC_EXECUTOR_OPERATOR_PHE_FIXED_POINT_ENCODER_H_
#define FALCON_SRC_EXECUTOR_OPERATOR_PHE_FIXED_POINT_ENCODER_H_

#include "libhcs.h"
#include "gmp.h"
#include "falcon/common.h"

/**
 * After decryption, there are four states for the decrypted number:
 *
 * Invalid : corrupted encoded number (should mod n if needed)
 * Positive: encoded number is positive
 * Negative: encoded number is negative, should minus n before decoding
 * Overflow: encoded number is overflowed
 */
enum EncodedNumberState {Invalid, Positive, Negative, Overflow};

/**
 * describe the type of the EncodedNumber, three types
 *
 * Plaintext: the original value or revealed secret shared value
 * Ciphertext: the encrypted value by threshold Damgard Jurik cryptosystem
 */
enum EncodedNumberType {Plaintext, Ciphertext};

// fixed pointed integer representation
class EncodedNumber {
 private:
  // max value in public key for encoding
  mpz_t n;
  // the value in mpz_t form
  mpz_t value;
  // 0 for integer, negative for float
  int exponent;
  // the encoded number type
  EncodedNumberType type;

 public:
  /**
   * default constructor
   */
  EncodedNumber();

  /**
   * copy constructor
   * @param number
   */
  EncodedNumber(const EncodedNumber &number);

  /**
   * copy assignment constructor
   * @param number
   * @return
   */
  EncodedNumber &operator=(const EncodedNumber &number);

  /**
   * destructor
   */
  ~EncodedNumber();

  /**
   * set for int value
   * @param pn
   * @param v
   */
  void set_integer(mpz_t pn, int v);

  /**
   * set for float value
   * @param pn
   * @param v
   * @param precision
   */
  void set_float(mpz_t pn, float v, int precision = PHE_FIXED_POINT_PRECISION);

  /**
     * make two EncodedNumber exponent the same for computation
     * on the same level, only reasonable when two are plaintexts
     * @param new_exponent
     */
  void decrease_exponent(int new_exponent);

  /**
     * given a mpz_t value, increase its exponent for control
     * NOTE: this function only work for plaintext and will lose some precision
     * should be used carefully
     * @param new_exponent
     */
  void increase_exponent(int new_exponent);

  /**
   * decode to long value
   * @deprecated
   * @param v
   */
  void decode(long & v);

  /**
   * decode to float value
   * @param v
   */
  void decode(float & v);

  /**
   * when exponent is large, decode with truncation
   * NOTE: should be careful that the decoded value maybe inaccurate,
   * especially when the value is small
   *
   * @param v
   * @param truncated_exponent
   */
  void decode_with_truncation(float & v, int truncated_exponent);

  /**
   * check encoded number
   * if value >= n, then return Invalid
   * else if value <= max_int then return positive
   * else if value >= n - max_int then return negative
   * else (max_int < value < n - max_int) then return overflow
   *
   * NOTE: this function only works for the plaintext after decryption,
   * if an encoded number is not encrypted and decrypted, it should always return
   * a positive state because we assume the plaintext will never
   * surpass the max_int of the djcs_t cryptosystem?
   *
   * @return
   */
  EncodedNumberState check_encoded_number();

  /**
   * compute n // 3 - 1 as max_int
   * @param max_int
   */
  void compute_decode_threshold(mpz_t max_int);

  /** set EncodedNumber max value n */
  void setter_n(mpz_t s_n);

  /** set EncodedNumber value*/
  void setter_value(mpz_t s_value);

  /** set EncodedNumber exponent */
  void setter_exponent(int s_exponent);

  /** set EncodedNumber type */
  void setter_type(EncodedNumberType s_type);

  /** get EncodedNumber max value */
  void getter_n(__mpz_struct *g_n) const;

  /** get EncodedNumber value */
  void getter_value(__mpz_struct *g_value) const;

  /** get EncodedNumber exponent*/
  int getter_exponent() const;

  /** get EncodedNumber type */
  EncodedNumberType  getter_type() const;
};

/**
 * represent a float with fixed pointed integer
 * @param value
 * @param precision : exponent with precision
 * @return
 */
long long fixed_pointed_integer_representation(float value, int precision);

/**
 * encode an integer with mpz_t
 * must ensure that abs(value) <= n / 3
 * @param value
 * @param res
 * @param exponent
 */
void fixed_pointed_encode(long value, mpz_t res, int & exponent);

/**
 * encode a float with mpz_t
 * must ensure that abs(value * PHE_FIXED_POINT_BASE ** precision) <= n / 3
 * @param value
 * @param precision
 * @param res
 * @param exponent
 */
void fixed_pointed_encode(float value, int precision, mpz_t res, int & exponent);

/**
 * decode a mpz_t to a long value when exponent is 0
 * @param value
 * @param res
 */
void fixed_pointed_decode(long & value, mpz_t res);

/**
 * decode a mpz_t to a float value when exponent is not 0
 * @param value
 * @param res
 * @param exponent
 */
void fixed_pointed_decode(float & value, mpz_t res, int exponent);

/**
 * decode a mpz_t to a float value when exponent is not 0 with precision truncation
 * NOTE: the truncation can only be applied on plaintexts
 * @param value
 * @param res
 * @param exponent
 * @param truncated_exponent : truncate the res to desired precision to avoid overflow
 */
void fixed_pointed_decode_truncated(float & value, mpz_t res, int exponent, int truncated_exponent);


#endif //FALCON_SRC_EXECUTOR_OPERATOR_PHE_FIXED_POINT_ENCODER_H_
