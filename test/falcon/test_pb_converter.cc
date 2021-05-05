//
// Created by wuyuncheng on 13/8/20.
//

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#include <falcon/utils/pb_converter/model_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_params_converter.h>
#include <falcon/utils/pb_converter/network_converter.h>

TEST(PB_Converter, ModelPublishRequest) {
  int model_id = 1;
  int initiator_party_id = 1001;
  std::string output_message;
  serialize_model_publish_request(model_id,
      initiator_party_id,
      output_message);

  int deserialized_model_id;
  int deserialized_initiator_party_id;
  deserialize_model_publish_request(deserialized_model_id,
      deserialized_initiator_party_id,
      output_message);

  EXPECT_EQ(1, deserialized_model_id);
  EXPECT_EQ(1001, deserialized_initiator_party_id);
}

TEST(PB_Converter, ModelPublishResponse) {
  int model_id = 1;
  int initiator_party_id = 1001;
  int is_success = 0;
  int error_code = 2001;
  std::string error_msg = "Model id does not exist.";
  std::string output_message;
  serialize_model_publish_response(model_id,
      initiator_party_id,
      is_success,
      error_code,
      error_msg,
      output_message);

  int deserialized_model_id;
  int deserialized_initiator_party_id;
  int deserialized_is_success;
  int deserialized_error_code;
  std::string deserialized_error_msg;
  deserialize_model_publish_response(deserialized_model_id,
      deserialized_initiator_party_id,
      deserialized_is_success,
      deserialized_error_code,
      deserialized_error_msg,
      output_message);

  EXPECT_EQ(1, deserialized_model_id);
  EXPECT_EQ(1001, deserialized_initiator_party_id);
  EXPECT_EQ(0, deserialized_is_success);
  EXPECT_EQ(2001, deserialized_error_code);
  EXPECT_TRUE(error_msg == deserialized_error_msg);
}

TEST(PB_Converter, PHEKeys) {
  // generate phe keys
  hcs_random* phe_random = hcs_init_random();
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  djcs_t_private_key* phe_priv_key = djcs_t_init_private_key();
  djcs_t_auth_server** phe_auth_server = (djcs_t_auth_server **) malloc (3 * sizeof(djcs_t_auth_server *));
  mpz_t* si = (mpz_t *) malloc (3 * sizeof(mpz_t));
  djcs_t_generate_key_pair(phe_pub_key, phe_priv_key, phe_random, 1, 1024, 3, 3);
  mpz_t* coeff = djcs_t_init_polynomial(phe_priv_key, phe_random);
  for (int i = 0; i < 3; i++) {
    mpz_init(si[i]);
    djcs_t_compute_polynomial(phe_priv_key, coeff, si[i], i);
    phe_auth_server[i] = djcs_t_init_auth_server();
    djcs_t_set_auth_server(phe_auth_server[i], si[i], i);
  }

  // test serialization and deserialization
  for(int i = 0; i < 3; i++) {
    std::string output_message;
    serialize_phe_keys(phe_pub_key, phe_auth_server[i], output_message);
    djcs_t_public_key* deserialized_phe_pub_key = djcs_t_init_public_key();
    djcs_t_auth_server* deserialized_phe_auth_server = djcs_t_init_auth_server();
    deserialize_phe_keys(deserialized_phe_pub_key, deserialized_phe_auth_server, output_message);

    EXPECT_EQ(phe_pub_key->s, deserialized_phe_pub_key->s);
    EXPECT_EQ(phe_pub_key->l, deserialized_phe_pub_key->l);
    EXPECT_EQ(phe_pub_key->w, deserialized_phe_pub_key->w);
    EXPECT_EQ(0, mpz_cmp(phe_pub_key->g, deserialized_phe_pub_key->g));
    EXPECT_EQ(0, mpz_cmp(phe_pub_key->delta, deserialized_phe_pub_key->delta));
    for (int j = 0; j < phe_pub_key->s + 1; j++) {
      EXPECT_EQ(0, mpz_cmp(phe_pub_key->n[j], deserialized_phe_pub_key->n[j]));
    }
    EXPECT_EQ(0, mpz_cmp(phe_auth_server[i]->si, deserialized_phe_auth_server->si));
    EXPECT_EQ(phe_auth_server[i]->i, deserialized_phe_auth_server->i);

    djcs_t_free_public_key(deserialized_phe_pub_key);
    djcs_t_free_auth_server(deserialized_phe_auth_server);
  }

  // free memory
  hcs_free_random(phe_random);
  djcs_t_free_public_key(phe_pub_key);
  djcs_t_free_private_key(phe_priv_key);
  djcs_t_free_polynomial(phe_priv_key, coeff);
  for (int i = 0; i < 3; i++) {
    mpz_clear(si[i]);
    djcs_t_free_auth_server(phe_auth_server[i]);
  }
  free(si);
  free(phe_auth_server);
}

TEST(PB_Converter, IntArray) {
  // generate int array
  std::vector<int> vec;
  vec.push_back(3);
  vec.push_back(1);
  vec.push_back(4);
  vec.push_back(5);
  vec.push_back(2);
  std::string output_message;
  serialize_int_array(vec, output_message);

  std::vector<int> deserialized_vec;
  deserialize_int_array(deserialized_vec, output_message);

  // check equality
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(vec[i], deserialized_vec[i]);
  }
}

TEST(PB_Converter, FixedPointEncodedNumber) {
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "100", 10);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;

  EncodedNumber number;
  number.setter_n(v_n);
  number.setter_value(v_value);
  number.setter_exponent(v_exponent);
  number.setter_type(v_type);

  std::string output_message;
  serialize_encoded_number(number, output_message);
  EncodedNumber deserialized_number;
  deserialize_encoded_number(deserialized_number, output_message);

  mpz_t deserialized_n, deserialized_value;
  mpz_init(deserialized_n);
  mpz_init(deserialized_value);
  deserialized_number.getter_n(deserialized_n);
  deserialized_number.getter_value(deserialized_value);
  int n_cmp = mpz_cmp(v_n, deserialized_n);
  int value_cmp = mpz_cmp(v_value, deserialized_value);

  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(v_exponent, deserialized_number.getter_exponent());
  EXPECT_EQ(v_type, deserialized_number.getter_type());

  mpz_clear(v_n);
  mpz_clear(v_value);
  mpz_clear(deserialized_n);
  mpz_clear(deserialized_value);
}

TEST(PB_Converter, EncodedNumberArray) {
  EncodedNumber* encoded_number_array = new EncodedNumber[3];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", 10);
  mpz_set_str(v_value, "100", 10);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < 3; i++) {
    encoded_number_array[i].setter_n(v_n);
    encoded_number_array[i].setter_value(v_value);
    encoded_number_array[i].setter_exponent(v_exponent);
    encoded_number_array[i].setter_type(v_type);
  }

  std::string out_message;
  serialize_encoded_number_array(encoded_number_array, 3, out_message);
  EncodedNumber* deserialized_number_array = new EncodedNumber[3];
  deserialize_encoded_number_array(deserialized_number_array, 3, out_message);

  for (int i = 0; i < 3; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    deserialized_number_array[i].getter_n(deserialized_n);
    deserialized_number_array[i].getter_value(deserialized_value);
    int n_cmp = mpz_cmp(v_n, deserialized_n);
    int value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_number_array[i].getter_exponent());
    EXPECT_EQ(v_type, deserialized_number_array[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }
  mpz_clear(v_n);
  mpz_clear(v_value);
  delete [] encoded_number_array;
  delete [] deserialized_number_array;
}

TEST(PB_Converter, LogisticRegressionParams) {
  LogisticRegressionParams lr_params;
  lr_params.batch_size = 32;
  lr_params.max_iteration = 100;
  lr_params.converge_threshold = 1e-3;
  lr_params.with_regularization = false;
  lr_params.alpha = 0.5;
  lr_params.learning_rate = 0.1;
  lr_params.decay = 0.8;
  lr_params.penalty = "l2";
  lr_params.optimizer = "sgd";
  lr_params.multi_class = "ovr";
  lr_params.metric = "acc";
  lr_params.dp_budget = 0;
  std::string output_message;
  serialize_lr_params(lr_params, output_message);

  LogisticRegressionParams deserialized_lr_params;
  deserialize_lr_params(deserialized_lr_params, output_message);
  EXPECT_EQ(lr_params.batch_size, deserialized_lr_params.batch_size);
  EXPECT_EQ(lr_params.max_iteration, deserialized_lr_params.max_iteration);
  EXPECT_EQ(lr_params.converge_threshold, deserialized_lr_params.converge_threshold);
  EXPECT_EQ(lr_params.with_regularization, deserialized_lr_params.with_regularization);
  EXPECT_EQ(lr_params.alpha, deserialized_lr_params.alpha);
  EXPECT_EQ(lr_params.learning_rate, deserialized_lr_params.learning_rate);
  EXPECT_EQ(lr_params.decay, deserialized_lr_params.decay);
  EXPECT_EQ(lr_params.dp_budget, deserialized_lr_params.dp_budget);
  EXPECT_TRUE(lr_params.penalty == deserialized_lr_params.penalty);
  EXPECT_TRUE(lr_params.optimizer == deserialized_lr_params.optimizer);
  EXPECT_TRUE(lr_params.multi_class == deserialized_lr_params.multi_class);
  EXPECT_TRUE(lr_params.metric == deserialized_lr_params.metric);
}

TEST(PB_Converter, NetworkConfig) {
  // serialization
  std::vector<std::string> ip_addresses, deserialized_ip_addresses;
  std::vector< std::vector<int> > parties_port_nums, deserialized_parties_port_nums;
  for (int i = 0; i < 3; i++) {
    ip_addresses.emplace_back("127.0.0.1");
    std::vector<int> ports;
    for (int j = 0; j < 3; j++) {
      ports.push_back(9000 + i * 20 + j);
    }
    parties_port_nums.push_back(ports);
  }
  std::string output_message;
  serialize_network_configs(ip_addresses,
      parties_port_nums,
      output_message);

  // deserialzation
  deserialize_network_configs(deserialized_ip_addresses,
      deserialized_parties_port_nums,
      output_message);
  for (int i = 0; i < 3; i++) {
    EXPECT_TRUE(ip_addresses[i] == deserialized_ip_addresses[i]);
    for (int j = 0; j < 3; j++) {
      EXPECT_EQ(parties_port_nums[i][j], deserialized_parties_port_nums[i][j]);
    }
  }
}


