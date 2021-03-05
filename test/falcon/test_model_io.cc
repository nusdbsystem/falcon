//
// Created by wuyuncheng on 3/2/21.
//

#include <vector>

#include <falcon/model/model_io.h>

#include <gtest/gtest.h>

TEST(Model_IO, SaveModel) {
  int weight_size = 5;
  EncodedNumber* encoded_model = new EncodedNumber[weight_size];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "100", 10);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < weight_size; i++) {
    encoded_model[i].setter_n(v_n);
    encoded_model[i].setter_value(v_value);
    encoded_model[i].setter_exponent(v_exponent);
    encoded_model[i].setter_type(v_type);
  }

  std::string save_model_file = "saved_model.txt";
  save_lr_model(encoded_model, weight_size, save_model_file);

  EncodedNumber* loaded_model = new EncodedNumber[weight_size];
  int loaded_weight_size = 0;
  load_lr_model(save_model_file, loaded_weight_size, loaded_model);

  EXPECT_EQ(weight_size, loaded_weight_size);

  for (int i = 0; i < weight_size; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    loaded_model[i].getter_n(deserialized_n);
    loaded_model[i].getter_value(deserialized_value);
    int n_cmp = mpz_cmp(v_n, deserialized_n);
    int value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, loaded_model[i].getter_exponent());
    EXPECT_EQ(v_type, loaded_model[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
  delete [] encoded_model;
  delete [] loaded_model;
}