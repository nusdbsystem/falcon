//
// Created by wuyuncheng on 16/8/20.
//

#ifndef FALCON_SRC_EXECUTOR_OPERATOR_PHE_FIXED_POINT_ENCODER_H_
#define FALCON_SRC_EXECUTOR_OPERATOR_PHE_FIXED_POINT_ENCODER_H_

#include "libhcs.h"
#include "gmp.h"

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
  mpz_t n;                 // max value in public key for encoding
  mpz_t value;             // the value in mpz_t form
  int exponent;            // 0 for integer, negative for float
  EncodedNumberType type;  // the encoded number type

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

  void setter_n(mpz_t s_n);
  void setter_value(mpz_t s_value);
  void setter_exponent(int s_exponent);
  void setter_type(EncodedNumberType s_type);
  void getter_n(__mpz_struct *g_n) const;
  void getter_value(__mpz_struct *g_value) const;
  int getter_exponent() const;
  EncodedNumberType  getter_type() const;
};

#endif //FALCON_SRC_EXECUTOR_OPERATOR_PHE_FIXED_POINT_ENCODER_H_
