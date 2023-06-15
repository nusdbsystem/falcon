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

//
// Created by wuyuncheng on 13/8/20.
//

#include <iostream>
#include <string>

#include <falcon/utils/base64.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/interpretability_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/model_converter.h>
#include <falcon/utils/pb_converter/network_converter.h>
#include <falcon/utils/pb_converter/nn_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <gtest/gtest.h>

TEST(PB_Converter, ModelPublishRequest) {
  int model_id = 1;
  int initiator_party_id = 1001;
  std::string output_message;
  serialize_model_publish_request(model_id, initiator_party_id, output_message);

  int deserialized_model_id;
  int deserialized_initiator_party_id;
  deserialize_model_publish_request(
      deserialized_model_id, deserialized_initiator_party_id, output_message);

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
  serialize_model_publish_response(model_id, initiator_party_id, is_success,
                                   error_code, error_msg, output_message);

  int deserialized_model_id;
  int deserialized_initiator_party_id;
  int deserialized_is_success;
  int deserialized_error_code;
  std::string deserialized_error_msg;
  deserialize_model_publish_response(
      deserialized_model_id, deserialized_initiator_party_id,
      deserialized_is_success, deserialized_error_code, deserialized_error_msg,
      output_message);

  EXPECT_EQ(1, deserialized_model_id);
  EXPECT_EQ(1001, deserialized_initiator_party_id);
  EXPECT_EQ(0, deserialized_is_success);
  EXPECT_EQ(2001, deserialized_error_code);
  EXPECT_TRUE(error_msg == deserialized_error_msg);
}

TEST(PB_Converter, PHEKeys) {
  // generate phe keys
  hcs_random *phe_random = hcs_init_random();
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  djcs_t_private_key *phe_priv_key = djcs_t_init_private_key();
  djcs_t_auth_server **phe_auth_server =
      (djcs_t_auth_server **)malloc(3 * sizeof(djcs_t_auth_server *));
  mpz_t *si = (mpz_t *)malloc(3 * sizeof(mpz_t));
  djcs_t_generate_key_pair(phe_pub_key, phe_priv_key, phe_random, 1, 1024, 3,
                           3);
  mpz_t *coeff = djcs_t_init_polynomial(phe_priv_key, phe_random);
  for (int i = 0; i < 3; i++) {
    mpz_init(si[i]);
    djcs_t_compute_polynomial(phe_priv_key, coeff, si[i], i);
    phe_auth_server[i] = djcs_t_init_auth_server();
    djcs_t_set_auth_server(phe_auth_server[i], si[i], i);
  }

  // test serialization and deserialization
  for (int i = 0; i < 3; i++) {
    std::string output_message;
    serialize_phe_keys(phe_pub_key, phe_auth_server[i], output_message);
    djcs_t_public_key *deserialized_phe_pub_key = djcs_t_init_public_key();
    djcs_t_auth_server *deserialized_phe_auth_server =
        djcs_t_init_auth_server();
    deserialize_phe_keys(deserialized_phe_pub_key, deserialized_phe_auth_server,
                         output_message);

    EXPECT_EQ(phe_pub_key->s, deserialized_phe_pub_key->s);
    EXPECT_EQ(phe_pub_key->l, deserialized_phe_pub_key->l);
    EXPECT_EQ(phe_pub_key->w, deserialized_phe_pub_key->w);
    EXPECT_EQ(0, mpz_cmp(phe_pub_key->g, deserialized_phe_pub_key->g));
    EXPECT_EQ(0, mpz_cmp(phe_pub_key->delta, deserialized_phe_pub_key->delta));
    for (int j = 0; j < phe_pub_key->s + 1; j++) {
      EXPECT_EQ(0, mpz_cmp(phe_pub_key->n[j], deserialized_phe_pub_key->n[j]));
    }
    EXPECT_EQ(
        0, mpz_cmp(phe_auth_server[i]->si, deserialized_phe_auth_server->si));
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

TEST(PB_Converter, DoubleArray) {
  // generate double array
  std::vector<double> vec;
  vec.push_back(3.0);
  vec.push_back(1.0);
  vec.push_back(4.0);
  vec.push_back(5.0);
  vec.push_back(2.0);
  std::string output_message;
  serialize_double_array(vec, output_message);

  std::vector<double> deserialized_vec;
  deserialize_double_array(deserialized_vec, output_message);

  // check equality
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(vec[i], deserialized_vec[i]);
  }
}

TEST(PB_Converter, DoubleMatrix) {
  // generate double matrix
  std::vector<std::vector<double>> mat;
  for (int i = 0; i < 3; i++) {
    std::vector<double> vec;
    vec.push_back(3.0);
    vec.push_back(1.0);
    vec.push_back(4.0);
    vec.push_back(5.0);
    vec.push_back(2.0);
    mat.push_back(vec);
  }
  std::string output_message;
  serialize_double_matrix(mat, output_message);

  std::vector<std::vector<double>> deserialized_mat;
  deserialize_double_matrix(deserialized_mat, output_message);

  // check equality
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      EXPECT_EQ(mat[i][j], deserialized_mat[i][j]);
    }
  }
}

TEST(PB_Converter, FixedPointEncodedNumber) {
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
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
  auto *encoded_number_array = new EncodedNumber[3];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
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
  auto *deserialized_number_array = new EncodedNumber[3];
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
  delete[] encoded_number_array;
  delete[] deserialized_number_array;
}

TEST(PB_Converter, EncodedNumberMatrix) {
  auto **encoded_number_matrix = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    encoded_number_matrix[i] = new EncodedNumber[2];
  }
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      encoded_number_matrix[i][j].setter_n(v_n);
      encoded_number_matrix[i][j].setter_value(v_value);
      encoded_number_matrix[i][j].setter_exponent(v_exponent);
      encoded_number_matrix[i][j].setter_type(v_type);
    }
  }

  std::string out_message;
  serialize_encoded_number_matrix(encoded_number_matrix, 3, 2, out_message);
  auto **deserialized_number_matrix = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    deserialized_number_matrix[i] = new EncodedNumber[2];
  }
  deserialize_encoded_number_matrix(deserialized_number_matrix, 3, 2,
                                    out_message);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      mpz_t deserialized_n, deserialized_value;
      mpz_init(deserialized_n);
      mpz_init(deserialized_value);
      deserialized_number_matrix[i][j].getter_n(deserialized_n);
      deserialized_number_matrix[i][j].getter_value(deserialized_value);
      int n_cmp = mpz_cmp(v_n, deserialized_n);
      int value_cmp = mpz_cmp(v_value, deserialized_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_number_matrix[i][j].getter_exponent());
      EXPECT_EQ(v_type, deserialized_number_matrix[i][j].getter_type());
      mpz_clear(deserialized_n);
      mpz_clear(deserialized_value);
    }
  }
  mpz_clear(v_n);
  mpz_clear(v_value);

  for (int i = 0; i < 3; i++) {
    delete[] encoded_number_matrix[i];
    delete[] deserialized_number_matrix[i];
  }
  delete[] encoded_number_matrix;
  delete[] deserialized_number_matrix;
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
  lr_params.fit_bias = true;
  std::string output_message;
  serialize_lr_params(lr_params, output_message);

  LogisticRegressionParams deserialized_lr_params;
  deserialize_lr_params(deserialized_lr_params, output_message);
  EXPECT_EQ(lr_params.batch_size, deserialized_lr_params.batch_size);
  EXPECT_EQ(lr_params.max_iteration, deserialized_lr_params.max_iteration);
  EXPECT_EQ(lr_params.converge_threshold,
            deserialized_lr_params.converge_threshold);
  EXPECT_EQ(lr_params.with_regularization,
            deserialized_lr_params.with_regularization);
  // std::cout << "deserialized_lr_params.with_regularization = " <<
  // deserialized_lr_params.with_regularization << std::endl;
  EXPECT_EQ(lr_params.alpha, deserialized_lr_params.alpha);
  EXPECT_EQ(lr_params.learning_rate, deserialized_lr_params.learning_rate);
  EXPECT_EQ(lr_params.decay, deserialized_lr_params.decay);
  EXPECT_EQ(lr_params.dp_budget, deserialized_lr_params.dp_budget);
  EXPECT_EQ(lr_params.fit_bias, deserialized_lr_params.fit_bias);
  // std::cout << "deserialized_lr_params.fit_bias = " <<
  // deserialized_lr_params.fit_bias << std::endl;
  EXPECT_TRUE(lr_params.penalty == deserialized_lr_params.penalty);
  EXPECT_TRUE(lr_params.optimizer == deserialized_lr_params.optimizer);
  EXPECT_TRUE(lr_params.multi_class == deserialized_lr_params.multi_class);
  EXPECT_TRUE(lr_params.metric == deserialized_lr_params.metric);
}

TEST(PB_Converter, LogisticRegressionModel) {
  LogisticRegressionModel lr_model;
  lr_model.weight_size = 3;
  lr_model.local_weights = new EncodedNumber[3];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < 3; i++) {
    lr_model.local_weights[i].setter_n(v_n);
    lr_model.local_weights[i].setter_value(v_value);
    lr_model.local_weights[i].setter_exponent(v_exponent);
    lr_model.local_weights[i].setter_type(v_type);
  }

  std::string out_message;
  serialize_lr_model(lr_model, out_message);
  LogisticRegressionModel deserialized_lr_model;
  deserialize_lr_model(deserialized_lr_model, out_message);

  EXPECT_EQ(lr_model.weight_size, deserialized_lr_model.weight_size);
  for (int i = 0; i < 3; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    deserialized_lr_model.local_weights[i].getter_n(deserialized_n);
    deserialized_lr_model.local_weights[i].getter_value(deserialized_value);
    int n_cmp = mpz_cmp(v_n, deserialized_n);
    int value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent,
              deserialized_lr_model.local_weights[i].getter_exponent());
    EXPECT_EQ(v_type, deserialized_lr_model.local_weights[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }
  mpz_clear(v_n);
  mpz_clear(v_value);
}

TEST(PB_Converter, LinearRegressionParams) {
  LinearRegressionParams lir_params;
  lir_params.batch_size = 32;
  lir_params.max_iteration = 100;
  lir_params.converge_threshold = 1e-3;
  lir_params.with_regularization = false;
  lir_params.alpha = 0.5;
  lir_params.learning_rate = 0.1;
  lir_params.decay = 0.8;
  lir_params.penalty = "l2";
  lir_params.optimizer = "sgd";
  lir_params.metric = "acc";
  lir_params.dp_budget = 0;
  lir_params.fit_bias = true;
  std::string output_message;
  serialize_lir_params(lir_params, output_message);

  LinearRegressionParams deserialized_lir_params;
  deserialize_lir_params(deserialized_lir_params, output_message);
  EXPECT_EQ(lir_params.batch_size, deserialized_lir_params.batch_size);
  EXPECT_EQ(lir_params.max_iteration, deserialized_lir_params.max_iteration);
  EXPECT_EQ(lir_params.converge_threshold,
            deserialized_lir_params.converge_threshold);
  EXPECT_EQ(lir_params.with_regularization,
            deserialized_lir_params.with_regularization);
  // std::cout << "deserialized_lr_params.with_regularization = " <<
  // deserialized_lr_params.with_regularization << std::endl;
  EXPECT_EQ(lir_params.alpha, deserialized_lir_params.alpha);
  EXPECT_EQ(lir_params.learning_rate, deserialized_lir_params.learning_rate);
  EXPECT_EQ(lir_params.decay, deserialized_lir_params.decay);
  EXPECT_EQ(lir_params.dp_budget, deserialized_lir_params.dp_budget);
  EXPECT_EQ(lir_params.fit_bias, deserialized_lir_params.fit_bias);
  // std::cout << "deserialized_lr_params.fit_bias = " <<
  // deserialized_lr_params.fit_bias << std::endl;
  EXPECT_TRUE(lir_params.penalty == deserialized_lir_params.penalty);
  EXPECT_TRUE(lir_params.optimizer == deserialized_lir_params.optimizer);
  EXPECT_TRUE(lir_params.metric == deserialized_lir_params.metric);
}

TEST(PB_Converter, DecisionTreeParams) {
  DecisionTreeParams dt_params;
  dt_params.class_num = 2;
  dt_params.max_depth = 5;
  dt_params.max_bins = 8;
  dt_params.min_samples_split = 5;
  dt_params.min_samples_leaf = 5;
  dt_params.max_leaf_nodes = 16;
  dt_params.min_impurity_decrease = 0.01;
  dt_params.min_impurity_split = 0.001;
  dt_params.tree_type = "classification";
  dt_params.criterion = "gini";
  dt_params.split_strategy = "best";
  dt_params.dp_budget = 0;
  std::string output_message;
  serialize_dt_params(dt_params, output_message);

  DecisionTreeParams deserialized_dt_params;
  deserialize_dt_params(deserialized_dt_params, output_message);
  EXPECT_EQ(dt_params.class_num, deserialized_dt_params.class_num);
  EXPECT_EQ(dt_params.max_depth, deserialized_dt_params.max_depth);
  EXPECT_EQ(dt_params.max_bins, deserialized_dt_params.max_bins);
  EXPECT_EQ(dt_params.min_samples_split,
            deserialized_dt_params.min_samples_split);
  EXPECT_EQ(dt_params.min_samples_leaf,
            deserialized_dt_params.min_samples_leaf);
  EXPECT_EQ(dt_params.max_leaf_nodes, deserialized_dt_params.max_leaf_nodes);
  EXPECT_EQ(dt_params.min_impurity_decrease,
            deserialized_dt_params.min_impurity_decrease);
  EXPECT_EQ(dt_params.min_impurity_split,
            deserialized_dt_params.min_impurity_split);
  EXPECT_EQ(dt_params.dp_budget, deserialized_dt_params.dp_budget);
  EXPECT_TRUE(dt_params.tree_type == deserialized_dt_params.tree_type);
  EXPECT_TRUE(dt_params.criterion == deserialized_dt_params.criterion);
  EXPECT_TRUE(dt_params.split_strategy ==
              deserialized_dt_params.split_strategy);
}

TEST(PB_Converter, RandomForestParams) {
  RandomForestParams rf_params;
  rf_params.n_estimator = 8;
  rf_params.sample_rate = 0.8;
  rf_params.dt_param.class_num = 2;
  rf_params.dt_param.max_depth = 5;
  rf_params.dt_param.max_bins = 8;
  rf_params.dt_param.min_samples_split = 5;
  rf_params.dt_param.min_samples_leaf = 5;
  rf_params.dt_param.max_leaf_nodes = 16;
  rf_params.dt_param.min_impurity_decrease = 0.01;
  rf_params.dt_param.min_impurity_split = 0.001;
  rf_params.dt_param.tree_type = "classification";
  rf_params.dt_param.criterion = "gini";
  rf_params.dt_param.split_strategy = "best";
  rf_params.dt_param.dp_budget = 0;
  std::string output_message;
  serialize_rf_params(rf_params, output_message);

  RandomForestParams deserialized_rf_params;
  deserialize_rf_params(deserialized_rf_params, output_message);
  EXPECT_EQ(rf_params.n_estimator, deserialized_rf_params.n_estimator);
  EXPECT_EQ(rf_params.sample_rate, deserialized_rf_params.sample_rate);
  EXPECT_EQ(rf_params.dt_param.class_num,
            deserialized_rf_params.dt_param.class_num);
  EXPECT_EQ(rf_params.dt_param.max_depth,
            deserialized_rf_params.dt_param.max_depth);
  EXPECT_EQ(rf_params.dt_param.max_bins,
            deserialized_rf_params.dt_param.max_bins);
  EXPECT_EQ(rf_params.dt_param.min_samples_split,
            deserialized_rf_params.dt_param.min_samples_split);
  EXPECT_EQ(rf_params.dt_param.min_samples_leaf,
            deserialized_rf_params.dt_param.min_samples_leaf);
  EXPECT_EQ(rf_params.dt_param.max_leaf_nodes,
            deserialized_rf_params.dt_param.max_leaf_nodes);
  EXPECT_EQ(rf_params.dt_param.min_impurity_decrease,
            deserialized_rf_params.dt_param.min_impurity_decrease);
  EXPECT_EQ(rf_params.dt_param.min_impurity_split,
            deserialized_rf_params.dt_param.min_impurity_split);
  EXPECT_EQ(rf_params.dt_param.dp_budget,
            deserialized_rf_params.dt_param.dp_budget);
  EXPECT_TRUE(rf_params.dt_param.tree_type ==
              deserialized_rf_params.dt_param.tree_type);
  EXPECT_TRUE(rf_params.dt_param.criterion ==
              deserialized_rf_params.dt_param.criterion);
  EXPECT_TRUE(rf_params.dt_param.split_strategy ==
              deserialized_rf_params.dt_param.split_strategy);
}

TEST(PB_Converter, GbdtParams) {
  GbdtParams gbdt_params;
  gbdt_params.n_estimator = 8;
  gbdt_params.loss = "exponential";
  gbdt_params.learning_rate = 1.0;
  gbdt_params.subsample = 1.0;
  gbdt_params.dt_param.class_num = 2;
  gbdt_params.dt_param.max_depth = 5;
  gbdt_params.dt_param.max_bins = 8;
  gbdt_params.dt_param.min_samples_split = 5;
  gbdt_params.dt_param.min_samples_leaf = 5;
  gbdt_params.dt_param.max_leaf_nodes = 16;
  gbdt_params.dt_param.min_impurity_decrease = 0.01;
  gbdt_params.dt_param.min_impurity_split = 0.001;
  gbdt_params.dt_param.tree_type = "classification";
  gbdt_params.dt_param.criterion = "gini";
  gbdt_params.dt_param.split_strategy = "best";
  gbdt_params.dt_param.dp_budget = 0;
  std::string output_message;
  serialize_gbdt_params(gbdt_params, output_message);

  GbdtParams deserialized_gbdt_params;
  deserialize_gbdt_params(deserialized_gbdt_params, output_message);
  EXPECT_EQ(gbdt_params.n_estimator, deserialized_gbdt_params.n_estimator);
  EXPECT_TRUE(gbdt_params.loss == deserialized_gbdt_params.loss);
  EXPECT_EQ(gbdt_params.learning_rate, deserialized_gbdt_params.learning_rate);
  EXPECT_EQ(gbdt_params.subsample, deserialized_gbdt_params.subsample);
  EXPECT_EQ(gbdt_params.dt_param.class_num,
            deserialized_gbdt_params.dt_param.class_num);
  EXPECT_EQ(gbdt_params.dt_param.max_depth,
            deserialized_gbdt_params.dt_param.max_depth);
  EXPECT_EQ(gbdt_params.dt_param.max_bins,
            deserialized_gbdt_params.dt_param.max_bins);
  EXPECT_EQ(gbdt_params.dt_param.min_samples_split,
            deserialized_gbdt_params.dt_param.min_samples_split);
  EXPECT_EQ(gbdt_params.dt_param.min_samples_leaf,
            deserialized_gbdt_params.dt_param.min_samples_leaf);
  EXPECT_EQ(gbdt_params.dt_param.max_leaf_nodes,
            deserialized_gbdt_params.dt_param.max_leaf_nodes);
  EXPECT_EQ(gbdt_params.dt_param.min_impurity_decrease,
            deserialized_gbdt_params.dt_param.min_impurity_decrease);
  EXPECT_EQ(gbdt_params.dt_param.min_impurity_split,
            deserialized_gbdt_params.dt_param.min_impurity_split);
  EXPECT_EQ(gbdt_params.dt_param.dp_budget,
            deserialized_gbdt_params.dt_param.dp_budget);
  EXPECT_TRUE(gbdt_params.dt_param.tree_type ==
              deserialized_gbdt_params.dt_param.tree_type);
  EXPECT_TRUE(gbdt_params.dt_param.criterion ==
              deserialized_gbdt_params.dt_param.criterion);
  EXPECT_TRUE(gbdt_params.dt_param.split_strategy ==
              deserialized_gbdt_params.dt_param.split_strategy);
}

TEST(PB_Converter, MlpParams) {
  MlpParams mlp_params;
  mlp_params.is_classification = true;
  mlp_params.batch_size = 32;
  mlp_params.max_iteration = 100;
  mlp_params.converge_threshold = 1e-3;
  mlp_params.with_regularization = false;
  mlp_params.alpha = 0.5;
  mlp_params.learning_rate = 0.1;
  mlp_params.decay = 0.8;
  mlp_params.penalty = "l2";
  mlp_params.optimizer = "sgd";
  mlp_params.metric = "acc";
  mlp_params.dp_budget = 0;
  mlp_params.fit_bias = true;
  mlp_params.num_layers_outputs.push_back(20);
  mlp_params.num_layers_outputs.push_back(100);
  mlp_params.num_layers_outputs.push_back(100);
  mlp_params.num_layers_outputs.push_back(2);
  mlp_params.layers_activation_funcs.emplace_back("sigmoid");
  mlp_params.layers_activation_funcs.emplace_back("sigmoid");
  mlp_params.layers_activation_funcs.emplace_back("linear");
  std::string output_message;
  serialize_mlp_params(mlp_params, output_message);

  MlpParams deserialized_mlp_params;
  deserialize_mlp_params(deserialized_mlp_params, output_message);
  EXPECT_EQ(mlp_params.is_classification,
            deserialized_mlp_params.is_classification);
  EXPECT_EQ(mlp_params.batch_size, deserialized_mlp_params.batch_size);
  EXPECT_EQ(mlp_params.max_iteration, deserialized_mlp_params.max_iteration);
  EXPECT_EQ(mlp_params.converge_threshold,
            deserialized_mlp_params.converge_threshold);
  EXPECT_EQ(mlp_params.with_regularization,
            deserialized_mlp_params.with_regularization);
  EXPECT_EQ(mlp_params.alpha, deserialized_mlp_params.alpha);
  EXPECT_EQ(mlp_params.learning_rate, deserialized_mlp_params.learning_rate);
  EXPECT_EQ(mlp_params.decay, deserialized_mlp_params.decay);
  EXPECT_EQ(mlp_params.dp_budget, deserialized_mlp_params.dp_budget);
  EXPECT_EQ(mlp_params.fit_bias, deserialized_mlp_params.fit_bias);
  EXPECT_TRUE(mlp_params.penalty == deserialized_mlp_params.penalty);
  EXPECT_TRUE(mlp_params.optimizer == deserialized_mlp_params.optimizer);
  EXPECT_TRUE(mlp_params.metric == deserialized_mlp_params.metric);
  int layer_size = (int)mlp_params.num_layers_outputs.size();
  int hidden_layer_size = (int)mlp_params.layers_activation_funcs.size();
  for (int i = 0; i < layer_size; i++) {
    EXPECT_EQ(mlp_params.num_layers_outputs[i],
              deserialized_mlp_params.num_layers_outputs[i]);
  }
  for (int i = 0; i < hidden_layer_size; i++) {
    EXPECT_TRUE(mlp_params.layers_activation_funcs[i] ==
                deserialized_mlp_params.layers_activation_funcs[i]);
  }
}

TEST(PB_Converter, TreeEncryptedStatistics) {
  int client_id = 0;
  int node_index = 1;
  int split_num = 3;
  int classes_num = 2;
  auto *left_sample_nums = new EncodedNumber[3];
  auto *right_sample_nums = new EncodedNumber[3];
  auto **encrypted_statistics = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    encrypted_statistics[i] = new EncodedNumber[2 * 2];
  }
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < 3; i++) {
    left_sample_nums[i].setter_n(v_n);
    left_sample_nums[i].setter_value(v_value);
    left_sample_nums[i].setter_exponent(v_exponent);
    left_sample_nums[i].setter_type(v_type);
    right_sample_nums[i].setter_n(v_n);
    right_sample_nums[i].setter_value(v_value);
    right_sample_nums[i].setter_exponent(v_exponent);
    right_sample_nums[i].setter_type(v_type);
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2 * 2; j++) {
      encrypted_statistics[i][j].setter_n(v_n);
      encrypted_statistics[i][j].setter_value(v_value);
      encrypted_statistics[i][j].setter_exponent(v_exponent);
      encrypted_statistics[i][j].setter_type(v_type);
    }
  }

  std::string output_message;
  serialize_encrypted_statistics(client_id, node_index, split_num, classes_num,
                                 left_sample_nums, right_sample_nums,
                                 encrypted_statistics, output_message);

  int deserialized_client_id = 0;
  int deserialized_node_index = 1;
  int deserialized_split_num = 3;
  int deserialized_classes_num = 2;
  auto *deserialized_left_sample_nums = new EncodedNumber[3];
  auto *deserialized_right_sample_nums = new EncodedNumber[3];
  auto **deserialized_encrypted_statistics = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    deserialized_encrypted_statistics[i] = new EncodedNumber[2 * 2];
  }

  deserialize_encrypted_statistics(
      deserialized_client_id, deserialized_node_index, deserialized_split_num,
      deserialized_classes_num, deserialized_left_sample_nums,
      deserialized_right_sample_nums, deserialized_encrypted_statistics,
      output_message);

  for (int i = 0; i < 3; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    deserialized_left_sample_nums[i].getter_n(deserialized_n);
    deserialized_left_sample_nums[i].getter_value(deserialized_value);
    int n_cmp = mpz_cmp(v_n, deserialized_n);
    int value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_left_sample_nums[i].getter_exponent());
    EXPECT_EQ(v_type, deserialized_left_sample_nums[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }

  for (int i = 0; i < 3; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    deserialized_right_sample_nums[i].getter_n(deserialized_n);
    deserialized_right_sample_nums[i].getter_value(deserialized_value);
    int n_cmp = mpz_cmp(v_n, deserialized_n);
    int value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_right_sample_nums[i].getter_exponent());
    EXPECT_EQ(v_type, deserialized_right_sample_nums[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2 * 2; j++) {
      mpz_t deserialized_n, deserialized_value;
      mpz_init(deserialized_n);
      mpz_init(deserialized_value);
      deserialized_encrypted_statistics[i][j].getter_n(deserialized_n);
      deserialized_encrypted_statistics[i][j].getter_value(deserialized_value);
      int n_cmp = mpz_cmp(v_n, deserialized_n);
      int value_cmp = mpz_cmp(v_value, deserialized_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent,
                deserialized_encrypted_statistics[i][j].getter_exponent());
      EXPECT_EQ(v_type, deserialized_encrypted_statistics[i][j].getter_type());
      mpz_clear(deserialized_n);
      mpz_clear(deserialized_value);
    }
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
  delete[] left_sample_nums;
  delete[] right_sample_nums;
  delete[] deserialized_left_sample_nums;
  delete[] deserialized_right_sample_nums;
  for (int i = 0; i < 3; i++) {
    delete[] encrypted_statistics[i];
    delete[] deserialized_encrypted_statistics[i];
  }
  delete[] encrypted_statistics;
  delete[] deserialized_encrypted_statistics;
}

TEST(PB_Converter, TreeUpdateInfo) {
  int source_party_id = 0;
  int best_party_id = 0;
  int best_feature_id = 1;
  int best_split_id = 2;
  EncodedNumber left_impurity, right_impurity;
  auto *left_sample_iv = new EncodedNumber[3];
  auto *right_sample_iv = new EncodedNumber[3];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  left_impurity.setter_n(v_n);
  left_impurity.setter_value(v_value);
  left_impurity.setter_exponent(v_exponent);
  left_impurity.setter_type(v_type);
  right_impurity.setter_n(v_n);
  right_impurity.setter_value(v_value);
  right_impurity.setter_exponent(v_exponent);
  right_impurity.setter_type(v_type);
  for (int i = 0; i < 3; i++) {
    left_sample_iv[i].setter_n(v_n);
    left_sample_iv[i].setter_value(v_value);
    left_sample_iv[i].setter_exponent(v_exponent);
    left_sample_iv[i].setter_type(v_type);
    right_sample_iv[i].setter_n(v_n);
    right_sample_iv[i].setter_value(v_value);
    right_sample_iv[i].setter_exponent(v_exponent);
    right_sample_iv[i].setter_type(v_type);
  }

  std::string out_message;
  serialize_update_info(source_party_id, best_party_id, best_feature_id,
                        best_split_id, left_impurity, right_impurity,
                        left_sample_iv, right_sample_iv, 3, out_message);
  int deserialized_source_party_id, deserialized_best_party_id;
  int deserialized_best_feature_id, deserialized_best_split_id;
  EncodedNumber deserialized_left_impurity, deserialized_right_impurity;
  auto *deserialized_left_sample_iv = new EncodedNumber[3];
  auto *deserialized_right_sample_iv = new EncodedNumber[3];
  deserialize_update_info(
      deserialized_source_party_id, deserialized_best_party_id,
      deserialized_best_feature_id, deserialized_best_split_id,
      deserialized_left_impurity, deserialized_right_impurity,
      deserialized_left_sample_iv, deserialized_right_sample_iv, out_message);

  EXPECT_EQ(source_party_id, deserialized_source_party_id);
  EXPECT_EQ(best_party_id, deserialized_best_party_id);
  EXPECT_EQ(best_feature_id, deserialized_best_feature_id);
  EXPECT_EQ(best_split_id, deserialized_best_split_id);

  mpz_t deserialized_left_n, deserialized_left_value;
  mpz_t deserialized_right_n, deserialized_right_value;
  mpz_init(deserialized_left_n);
  mpz_init(deserialized_left_value);
  mpz_init(deserialized_right_n);
  mpz_init(deserialized_right_value);

  deserialized_left_impurity.getter_n(deserialized_left_n);
  deserialized_left_impurity.getter_value(deserialized_left_value);
  int n_cmp = mpz_cmp(v_n, deserialized_left_n);
  int value_cmp = mpz_cmp(v_value, deserialized_left_value);
  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(v_exponent, deserialized_left_impurity.getter_exponent());
  EXPECT_EQ(v_type, deserialized_left_impurity.getter_type());

  deserialized_right_impurity.getter_n(deserialized_right_n);
  deserialized_right_impurity.getter_value(deserialized_right_value);
  n_cmp = mpz_cmp(v_n, deserialized_right_n);
  value_cmp = mpz_cmp(v_value, deserialized_right_value);
  EXPECT_EQ(0, n_cmp);
  EXPECT_EQ(0, value_cmp);
  EXPECT_EQ(v_exponent, deserialized_right_impurity.getter_exponent());
  EXPECT_EQ(v_type, deserialized_right_impurity.getter_type());

  for (int i = 0; i < 3; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    deserialized_left_sample_iv[i].getter_n(deserialized_n);
    deserialized_left_sample_iv[i].getter_value(deserialized_value);
    n_cmp = mpz_cmp(v_n, deserialized_n);
    value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_left_sample_iv[i].getter_exponent());
    EXPECT_EQ(v_type, deserialized_left_sample_iv[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }

  for (int i = 0; i < 3; i++) {
    mpz_t deserialized_n, deserialized_value;
    mpz_init(deserialized_n);
    mpz_init(deserialized_value);
    deserialized_right_sample_iv[i].getter_n(deserialized_n);
    deserialized_right_sample_iv[i].getter_value(deserialized_value);
    n_cmp = mpz_cmp(v_n, deserialized_n);
    value_cmp = mpz_cmp(v_value, deserialized_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_right_sample_iv[i].getter_exponent());
    EXPECT_EQ(v_type, deserialized_right_sample_iv[i].getter_type());
    mpz_clear(deserialized_n);
    mpz_clear(deserialized_value);
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
  mpz_clear(deserialized_left_n);
  mpz_clear(deserialized_left_value);
  mpz_clear(deserialized_right_n);
  mpz_clear(deserialized_right_value);
  delete[] left_sample_iv;
  delete[] right_sample_iv;
  delete[] deserialized_left_sample_iv;
  delete[] deserialized_right_sample_iv;
}

TEST(PB_Converter, TreeSplitInfo) {
  int global_split_num = 6;
  std::vector<int> client_split_nums;
  client_split_nums.push_back(1);
  client_split_nums.push_back(2);
  client_split_nums.push_back(3);
  std::string output_message;

  serialize_split_info(global_split_num, client_split_nums, output_message);

  int deserialized_global_split_num;
  std::vector<int> deserialized_client_split_nums;
  deserialize_split_info(deserialized_global_split_num,
                         deserialized_client_split_nums, output_message);

  EXPECT_EQ(global_split_num, deserialized_global_split_num);
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(client_split_nums[i], deserialized_client_split_nums[i]);
  }
}

TEST(PB_Converter, TreeModel) {
  TreeModel tree;
  tree.type = falcon::CLASSIFICATION;
  tree.class_num = 2;
  tree.max_depth = 2;
  tree.internal_node_num = 3;
  tree.total_node_num = 7;
  tree.capacity = 7;
  tree.nodes = new Node[tree.capacity];
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < tree.capacity; i++) {
    if (i < 7) {
      tree.nodes[i].node_type = falcon::INTERNAL;
    } else {
      tree.nodes[i].node_type = falcon::LEAF;
    }
    tree.nodes[i].depth = 1;
    tree.nodes[i].is_self_feature = 1;
    tree.nodes[i].best_party_id = 0;
    tree.nodes[i].best_feature_id = 1;
    tree.nodes[i].best_split_id = 2;
    tree.nodes[i].split_threshold = 0.25;
    tree.nodes[i].node_sample_num = 1000;
    tree.nodes[i].node_sample_distribution.push_back(200);
    tree.nodes[i].node_sample_distribution.push_back(300);
    tree.nodes[i].node_sample_distribution.push_back(500);
    tree.nodes[i].left_child = 2 * i + 1;
    tree.nodes[i].right_child = 2 * i + 2;
    EncodedNumber impurity, label;
    impurity.setter_n(v_n);
    impurity.setter_value(v_value);
    impurity.setter_exponent(v_exponent);
    impurity.setter_type(v_type);
    label.setter_n(v_n);
    label.setter_value(v_value);
    label.setter_exponent(v_exponent);
    label.setter_type(v_type);
    tree.nodes[i].impurity = impurity;
    tree.nodes[i].label = label;
  }

  std::string out_message;
  serialize_tree_model(tree, out_message);

  TreeModel deserialized_tree;
  deserialize_tree_model(deserialized_tree, out_message);

  EXPECT_EQ(deserialized_tree.type, tree.type);
  EXPECT_EQ(deserialized_tree.class_num, tree.class_num);
  EXPECT_EQ(deserialized_tree.max_depth, tree.max_depth);
  EXPECT_EQ(deserialized_tree.internal_node_num, tree.internal_node_num);
  EXPECT_EQ(deserialized_tree.total_node_num, tree.total_node_num);
  EXPECT_EQ(deserialized_tree.capacity, tree.capacity);
  for (int i = 0; i < tree.capacity; i++) {
    EXPECT_EQ(deserialized_tree.nodes[i].node_type, tree.nodes[i].node_type);
    EXPECT_EQ(deserialized_tree.nodes[i].depth, tree.nodes[i].depth);
    EXPECT_EQ(deserialized_tree.nodes[i].is_self_feature,
              tree.nodes[i].is_self_feature);
    EXPECT_EQ(deserialized_tree.nodes[i].best_party_id,
              tree.nodes[i].best_party_id);
    EXPECT_EQ(deserialized_tree.nodes[i].best_feature_id,
              tree.nodes[i].best_feature_id);
    EXPECT_EQ(deserialized_tree.nodes[i].best_split_id,
              tree.nodes[i].best_split_id);
    EXPECT_EQ(deserialized_tree.nodes[i].split_threshold,
              tree.nodes[i].split_threshold);
    EXPECT_EQ(deserialized_tree.nodes[i].node_sample_num,
              tree.nodes[i].node_sample_num);
    EXPECT_EQ(deserialized_tree.nodes[i].left_child, tree.nodes[i].left_child);
    EXPECT_EQ(deserialized_tree.nodes[i].right_child,
              tree.nodes[i].right_child);
    for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
      EXPECT_EQ(deserialized_tree.nodes[i].node_sample_distribution[j],
                tree.nodes[i].node_sample_distribution[j]);
    }

    EncodedNumber deserialized_impurity = deserialized_tree.nodes[i].impurity;
    EncodedNumber deserialized_label = deserialized_tree.nodes[i].label;
    mpz_t deserialized_impurity_n, deserialized_impurity_value;
    mpz_t deserialized_label_n, deserialized_label_value;
    mpz_init(deserialized_impurity_n);
    mpz_init(deserialized_impurity_value);
    mpz_init(deserialized_label_n);
    mpz_init(deserialized_label_value);

    deserialized_impurity.getter_n(deserialized_impurity_n);
    deserialized_impurity.getter_value(deserialized_impurity_value);
    int n_cmp = mpz_cmp(v_n, deserialized_impurity_n);
    int value_cmp = mpz_cmp(v_value, deserialized_impurity_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_impurity.getter_exponent());
    EXPECT_EQ(v_type, deserialized_impurity.getter_type());

    deserialized_label.getter_n(deserialized_label_n);
    deserialized_label.getter_value(deserialized_label_value);
    n_cmp = mpz_cmp(v_n, deserialized_label_n);
    value_cmp = mpz_cmp(v_value, deserialized_label_value);
    EXPECT_EQ(0, n_cmp);
    EXPECT_EQ(0, value_cmp);
    EXPECT_EQ(v_exponent, deserialized_label.getter_exponent());
    EXPECT_EQ(v_type, deserialized_label.getter_type());

    mpz_clear(deserialized_impurity_n);
    mpz_clear(deserialized_impurity_value);
    mpz_clear(deserialized_label_n);
    mpz_clear(deserialized_label_value);
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
}

TEST(PB_Converter, RandomForestModel) {
  ForestModel forest_model;
  forest_model.tree_size = 3;
  forest_model.tree_type = falcon::CLASSIFICATION;
  int n_estimator = 3;
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  TreeModel tree;
  tree.type = falcon::CLASSIFICATION;
  tree.class_num = 2;
  tree.max_depth = 2;
  tree.internal_node_num = 3;
  tree.total_node_num = 7;
  tree.capacity = 7;
  tree.nodes = new Node[tree.capacity];
  for (int i = 0; i < tree.capacity; i++) {
    if (i < 7) {
      tree.nodes[i].node_type = falcon::INTERNAL;
    } else {
      tree.nodes[i].node_type = falcon::LEAF;
    }
    tree.nodes[i].depth = 1;
    tree.nodes[i].is_self_feature = 1;
    tree.nodes[i].best_party_id = 0;
    tree.nodes[i].best_feature_id = 1;
    tree.nodes[i].best_split_id = 2;
    tree.nodes[i].split_threshold = 0.25;
    tree.nodes[i].node_sample_num = 1000;
    tree.nodes[i].node_sample_distribution.push_back(200);
    tree.nodes[i].node_sample_distribution.push_back(300);
    tree.nodes[i].node_sample_distribution.push_back(500);
    tree.nodes[i].left_child = 2 * i + 1;
    tree.nodes[i].right_child = 2 * i + 2;
    EncodedNumber impurity, label;
    impurity.setter_n(v_n);
    impurity.setter_value(v_value);
    impurity.setter_exponent(v_exponent);
    impurity.setter_type(v_type);
    label.setter_n(v_n);
    label.setter_value(v_value);
    label.setter_exponent(v_exponent);
    label.setter_type(v_type);
    tree.nodes[i].impurity = impurity;
    tree.nodes[i].label = label;
  }
  for (int t = 0; t < n_estimator; t++) {
    forest_model.forest_trees.push_back(tree);
  }

  std::string out_message;
  serialize_random_forest_model(forest_model, out_message);

  ForestModel deserialized_forest_model;
  deserialize_random_forest_model(deserialized_forest_model, out_message);
  EXPECT_EQ(forest_model.tree_size, deserialized_forest_model.tree_size);
  EXPECT_EQ(forest_model.tree_type, deserialized_forest_model.tree_type);
  for (int t = 0; t < deserialized_forest_model.tree_size; t++) {
    EXPECT_EQ(deserialized_forest_model.forest_trees[t].type, tree.type);
    EXPECT_EQ(deserialized_forest_model.forest_trees[t].class_num,
              tree.class_num);
    EXPECT_EQ(deserialized_forest_model.forest_trees[t].max_depth,
              tree.max_depth);
    EXPECT_EQ(deserialized_forest_model.forest_trees[t].internal_node_num,
              tree.internal_node_num);
    EXPECT_EQ(deserialized_forest_model.forest_trees[t].total_node_num,
              tree.total_node_num);
    EXPECT_EQ(deserialized_forest_model.forest_trees[t].capacity,
              tree.capacity);
    for (int i = 0; i < tree.capacity; i++) {
      EXPECT_EQ(deserialized_forest_model.forest_trees[t].nodes[i].node_type,
                tree.nodes[i].node_type);
      EXPECT_EQ(deserialized_forest_model.forest_trees[t].nodes[i].depth,
                tree.nodes[i].depth);
      EXPECT_EQ(
          deserialized_forest_model.forest_trees[t].nodes[i].is_self_feature,
          tree.nodes[i].is_self_feature);
      EXPECT_EQ(
          deserialized_forest_model.forest_trees[t].nodes[i].best_party_id,
          tree.nodes[i].best_party_id);
      EXPECT_EQ(
          deserialized_forest_model.forest_trees[t].nodes[i].best_feature_id,
          tree.nodes[i].best_feature_id);
      EXPECT_EQ(
          deserialized_forest_model.forest_trees[t].nodes[i].best_split_id,
          tree.nodes[i].best_split_id);
      EXPECT_EQ(
          deserialized_forest_model.forest_trees[t].nodes[i].split_threshold,
          tree.nodes[i].split_threshold);
      EXPECT_EQ(
          deserialized_forest_model.forest_trees[t].nodes[i].node_sample_num,
          tree.nodes[i].node_sample_num);
      EXPECT_EQ(deserialized_forest_model.forest_trees[t].nodes[i].left_child,
                tree.nodes[i].left_child);
      EXPECT_EQ(deserialized_forest_model.forest_trees[t].nodes[i].right_child,
                tree.nodes[i].right_child);
      for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
        EXPECT_EQ(deserialized_forest_model.forest_trees[t]
                      .nodes[i]
                      .node_sample_distribution[j],
                  tree.nodes[i].node_sample_distribution[j]);
      }

      EncodedNumber deserialized_impurity =
          deserialized_forest_model.forest_trees[t].nodes[i].impurity;
      EncodedNumber deserialized_label =
          deserialized_forest_model.forest_trees[t].nodes[i].label;
      mpz_t deserialized_impurity_n, deserialized_impurity_value;
      mpz_t deserialized_label_n, deserialized_label_value;
      mpz_init(deserialized_impurity_n);
      mpz_init(deserialized_impurity_value);
      mpz_init(deserialized_label_n);
      mpz_init(deserialized_label_value);

      deserialized_impurity.getter_n(deserialized_impurity_n);
      deserialized_impurity.getter_value(deserialized_impurity_value);
      int n_cmp = mpz_cmp(v_n, deserialized_impurity_n);
      int value_cmp = mpz_cmp(v_value, deserialized_impurity_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_impurity.getter_exponent());
      EXPECT_EQ(v_type, deserialized_impurity.getter_type());

      deserialized_label.getter_n(deserialized_label_n);
      deserialized_label.getter_value(deserialized_label_value);
      n_cmp = mpz_cmp(v_n, deserialized_label_n);
      value_cmp = mpz_cmp(v_value, deserialized_label_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_label.getter_exponent());
      EXPECT_EQ(v_type, deserialized_label.getter_type());

      mpz_clear(deserialized_impurity_n);
      mpz_clear(deserialized_impurity_value);
      mpz_clear(deserialized_label_n);
      mpz_clear(deserialized_label_value);
    }
  }
  mpz_clear(v_n);
  mpz_clear(v_value);
}

TEST(PB_Converter, GbdtModel) {
  GbdtModel gbdt_model;
  gbdt_model.tree_size = 6;
  gbdt_model.tree_type = falcon::CLASSIFICATION;
  gbdt_model.n_estimator = 3;
  gbdt_model.class_num = 2;
  gbdt_model.dummy_predictors.emplace_back(0.1);
  gbdt_model.learning_rate = 0.1;
  int n_estimator = 3;
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  TreeModel tree;
  tree.type = falcon::CLASSIFICATION;
  tree.class_num = 2;
  tree.max_depth = 2;
  tree.internal_node_num = 3;
  tree.total_node_num = 7;
  tree.capacity = 7;
  tree.nodes = new Node[tree.capacity];
  for (int i = 0; i < tree.capacity; i++) {
    if (i < 7) {
      tree.nodes[i].node_type = falcon::INTERNAL;
    } else {
      tree.nodes[i].node_type = falcon::LEAF;
    }
    tree.nodes[i].depth = 1;
    tree.nodes[i].is_self_feature = 1;
    tree.nodes[i].best_party_id = 0;
    tree.nodes[i].best_feature_id = 1;
    tree.nodes[i].best_split_id = 2;
    tree.nodes[i].split_threshold = 0.25;
    tree.nodes[i].node_sample_num = 1000;
    tree.nodes[i].node_sample_distribution.push_back(200);
    tree.nodes[i].node_sample_distribution.push_back(300);
    tree.nodes[i].node_sample_distribution.push_back(500);
    tree.nodes[i].left_child = 2 * i + 1;
    tree.nodes[i].right_child = 2 * i + 2;
    EncodedNumber impurity, label;
    impurity.setter_n(v_n);
    impurity.setter_value(v_value);
    impurity.setter_exponent(v_exponent);
    impurity.setter_type(v_type);
    label.setter_n(v_n);
    label.setter_value(v_value);
    label.setter_exponent(v_exponent);
    label.setter_type(v_type);
    tree.nodes[i].impurity = impurity;
    tree.nodes[i].label = label;
  }
  for (int t = 0; t < gbdt_model.tree_size; t++) {
    gbdt_model.gbdt_trees.push_back(tree);
  }

  std::string out_message;
  serialize_gbdt_model(gbdt_model, out_message);

  GbdtModel deserialized_gbdt_model;
  deserialize_gbdt_model(deserialized_gbdt_model, out_message);
  EXPECT_EQ(gbdt_model.tree_size, deserialized_gbdt_model.tree_size);
  EXPECT_EQ(gbdt_model.tree_type, deserialized_gbdt_model.tree_type);
  EXPECT_EQ(gbdt_model.n_estimator, deserialized_gbdt_model.n_estimator);
  EXPECT_EQ(gbdt_model.class_num, deserialized_gbdt_model.class_num);
  EXPECT_EQ(gbdt_model.learning_rate, deserialized_gbdt_model.learning_rate);
  for (int i = 0; i < gbdt_model.dummy_predictors.size(); i++) {
    EXPECT_EQ(gbdt_model.dummy_predictors[i],
              deserialized_gbdt_model.dummy_predictors[i]);
  }
  for (int t = 0; t < deserialized_gbdt_model.tree_size; t++) {
    EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].type, tree.type);
    EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].class_num, tree.class_num);
    EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].max_depth, tree.max_depth);
    EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].internal_node_num,
              tree.internal_node_num);
    EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].total_node_num,
              tree.total_node_num);
    EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].capacity, tree.capacity);
    for (int i = 0; i < tree.capacity; i++) {
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].node_type,
                tree.nodes[i].node_type);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].depth,
                tree.nodes[i].depth);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].is_self_feature,
                tree.nodes[i].is_self_feature);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].best_party_id,
                tree.nodes[i].best_party_id);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].best_feature_id,
                tree.nodes[i].best_feature_id);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].best_split_id,
                tree.nodes[i].best_split_id);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].split_threshold,
                tree.nodes[i].split_threshold);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].node_sample_num,
                tree.nodes[i].node_sample_num);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].left_child,
                tree.nodes[i].left_child);
      EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t].nodes[i].right_child,
                tree.nodes[i].right_child);
      for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
        EXPECT_EQ(deserialized_gbdt_model.gbdt_trees[t]
                      .nodes[i]
                      .node_sample_distribution[j],
                  tree.nodes[i].node_sample_distribution[j]);
      }

      EncodedNumber deserialized_impurity =
          deserialized_gbdt_model.gbdt_trees[t].nodes[i].impurity;
      EncodedNumber deserialized_label =
          deserialized_gbdt_model.gbdt_trees[t].nodes[i].label;
      mpz_t deserialized_impurity_n, deserialized_impurity_value;
      mpz_t deserialized_label_n, deserialized_label_value;
      mpz_init(deserialized_impurity_n);
      mpz_init(deserialized_impurity_value);
      mpz_init(deserialized_label_n);
      mpz_init(deserialized_label_value);

      deserialized_impurity.getter_n(deserialized_impurity_n);
      deserialized_impurity.getter_value(deserialized_impurity_value);
      int n_cmp = mpz_cmp(v_n, deserialized_impurity_n);
      int value_cmp = mpz_cmp(v_value, deserialized_impurity_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_impurity.getter_exponent());
      EXPECT_EQ(v_type, deserialized_impurity.getter_type());

      deserialized_label.getter_n(deserialized_label_n);
      deserialized_label.getter_value(deserialized_label_value);
      n_cmp = mpz_cmp(v_n, deserialized_label_n);
      value_cmp = mpz_cmp(v_value, deserialized_label_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_label.getter_exponent());
      EXPECT_EQ(v_type, deserialized_label.getter_type());

      mpz_clear(deserialized_impurity_n);
      mpz_clear(deserialized_impurity_value);
      mpz_clear(deserialized_label_n);
      mpz_clear(deserialized_label_value);
    }
  }
  mpz_clear(v_n);
  mpz_clear(v_value);
}

TEST(PB_Converter, MlpModel) {
  std::vector<int> num_layers_neurons;
  num_layers_neurons.push_back(3);
  num_layers_neurons.push_back(3);
  num_layers_neurons.push_back(3);
  std::vector<std::string> activation_func_str;
  activation_func_str.emplace_back("sigmoid");
  activation_func_str.emplace_back("sigmoid");
  MlpModel mlp_model(true, true, num_layers_neurons, activation_func_str);
  //  mlp_model.m_num_inputs = 3;
  //  mlp_model.m_num_outputs = 2;
  //  mlp_model.m_num_hidden_layers = 2;
  //  mlp_model.m_num_layers_neurons.push_back(3);
  //  mlp_model.m_num_layers_neurons.push_back(3);
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int i = 0; i < 2; i++) {
    //    Layer layer(3, 3, true, "sigmoid");
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        mlp_model.m_layers[i].m_weight_mat[j][k].setter_n(v_n);
        mlp_model.m_layers[i].m_weight_mat[j][k].setter_value(v_value);
        mlp_model.m_layers[i].m_weight_mat[j][k].setter_exponent(v_exponent);
        mlp_model.m_layers[i].m_weight_mat[j][k].setter_type(v_type);
      }
    }
    for (int k = 0; k < 3; k++) {
      mlp_model.m_layers[i].m_bias[k].setter_n(v_n);
      mlp_model.m_layers[i].m_bias[k].setter_value(v_value);
      mlp_model.m_layers[i].m_bias[k].setter_exponent(v_exponent);
      mlp_model.m_layers[i].m_bias[k].setter_type(v_type);
    }
  }

  std::string pb_str;
  serialize_mlp_model(mlp_model, pb_str);

  MlpModel des_mlp_model;
  deserialize_mlp_model(des_mlp_model, pb_str);

  EXPECT_EQ(mlp_model.m_is_classification, des_mlp_model.m_is_classification);
  EXPECT_EQ(mlp_model.m_num_inputs, des_mlp_model.m_num_inputs);
  EXPECT_EQ(mlp_model.m_num_outputs, des_mlp_model.m_num_outputs);
  EXPECT_EQ(mlp_model.m_num_hidden_layers, des_mlp_model.m_num_hidden_layers);
  EXPECT_EQ(mlp_model.m_layers_num_outputs.size(),
            des_mlp_model.m_layers_num_outputs.size());
  EXPECT_EQ(mlp_model.m_layers.size(), des_mlp_model.m_layers.size());
  for (int i = 0; i < mlp_model.m_layers.size(); i++) {
    EXPECT_EQ(mlp_model.m_layers[i].m_num_inputs,
              des_mlp_model.m_layers[i].m_num_inputs);
    EXPECT_EQ(mlp_model.m_layers[i].m_num_outputs,
              des_mlp_model.m_layers[i].m_num_outputs);
    EXPECT_EQ(mlp_model.m_layers[i].m_fit_bias,
              des_mlp_model.m_layers[i].m_fit_bias);
    EXPECT_TRUE(mlp_model.m_layers[i].m_activation_func_str ==
                des_mlp_model.m_layers[i].m_activation_func_str);
    for (int j = 0; j < mlp_model.m_layers[i].m_num_inputs; j++) {
      for (int k = 0; k < mlp_model.m_layers[i].m_num_outputs; k++) {
        mpz_t deserialized_n, deserialized_value;
        mpz_init(deserialized_n);
        mpz_init(deserialized_value);
        mlp_model.m_layers[i].m_weight_mat[j][k].getter_n(deserialized_n);
        mlp_model.m_layers[i].m_weight_mat[j][k].getter_value(
            deserialized_value);
        int n_cmp = mpz_cmp(v_n, deserialized_n);
        int value_cmp = mpz_cmp(v_value, deserialized_value);
        EXPECT_EQ(0, n_cmp);
        EXPECT_EQ(0, value_cmp);
        EXPECT_EQ(v_exponent,
                  mlp_model.m_layers[i].m_weight_mat[j][k].getter_exponent());
        EXPECT_EQ(v_type,
                  mlp_model.m_layers[i].m_weight_mat[j][k].getter_type());
        mpz_clear(deserialized_n);
        mpz_clear(deserialized_value);
      }
    }

    for (int k = 0; k < mlp_model.m_layers[i].m_num_outputs; k++) {
      // compare bias terms
      mpz_t des_n, des_value;
      mpz_init(des_n);
      mpz_init(des_value);
      mlp_model.m_layers[i].m_bias[k].getter_n(des_n);
      mlp_model.m_layers[i].m_bias[k].getter_value(des_value);
      int n_cmp = mpz_cmp(v_n, des_n);
      int value_cmp = mpz_cmp(v_value, des_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, mlp_model.m_layers[i].m_bias[k].getter_exponent());
      EXPECT_EQ(v_type, mlp_model.m_layers[i].m_bias[k].getter_type());
      mpz_clear(des_n);
      mpz_clear(des_value);
    }
  }
  EXPECT_EQ(mlp_model.m_n_layers, des_mlp_model.m_n_layers);

  mpz_clear(v_n);
  mpz_clear(v_value);
}

TEST(PB_Converter, NetworkConfig) {
  // serialization
  std::vector<std::string> ip_addresses, deserialized_ip_addresses;
  std::vector<std::vector<int>> executors_executors_port_nums,
      deserialized_executors_executors_port_nums;
  std::vector<int> executor_mpc_port_nums, deserialized_executor_mpc_port_nums;
  for (int i = 0; i < 3; i++) {
    ip_addresses.emplace_back("127.0.0.1");
    std::vector<int> ports;
    for (int j = 0; j < 3; j++) {
      ports.push_back(9000 + i * 20 + j);
    }
    executors_executors_port_nums.push_back(ports);
    executor_mpc_port_nums.push_back(10000 + i * 50);
  }
  std::string output_message;
  serialize_network_configs(ip_addresses, executors_executors_port_nums,
                            executor_mpc_port_nums, output_message);

  // deserialzation
  deserialize_network_configs(
      deserialized_ip_addresses, deserialized_executors_executors_port_nums,
      deserialized_executor_mpc_port_nums, output_message);
  for (int i = 0; i < 3; i++) {
    EXPECT_TRUE(ip_addresses[i] == deserialized_ip_addresses[i]);
    for (int j = 0; j < 3; j++) {
      EXPECT_EQ(executors_executors_port_nums[i][j],
                deserialized_executors_executors_port_nums[i][j]);
    }
    EXPECT_EQ(executor_mpc_port_nums[i],
              deserialized_executor_mpc_port_nums[i]);
  }
}

TEST(PB_Converter, PSNetworkConfig) {
  // serialization
  std::vector<std::string> worker_ips, deserialized_worker_ips;
  std::vector<std::string> ps_ips, deserialized_ps_ips;
  std::vector<int> worker_ports, deserialized_worker_ports;
  std::vector<int> ps_ports, deserialized_ps_ports;
  for (int i = 0; i < 3; i++) {
    worker_ips.emplace_back("127.0.0.1");
    worker_ports.emplace_back(9000 + i * 10);
    ps_ips.emplace_back("127.0.0.1");
    ps_ports.emplace_back(8000 + i * 20);
  }
  std::string output_message;
  serialize_ps_network_configs(worker_ips, worker_ports, ps_ips, ps_ports,
                               output_message);

  // deserialzation
  deserialize_ps_network_configs(deserialized_worker_ips,
                                 deserialized_worker_ports, deserialized_ps_ips,
                                 deserialized_ps_ports, output_message);
  for (int i = 0; i < 3; i++) {
    EXPECT_TRUE(worker_ips[i] == deserialized_worker_ips[i]);
    EXPECT_TRUE(ps_ips[i] == deserialized_ps_ips[i]);
    EXPECT_EQ(worker_ports[i], deserialized_worker_ports[i]);
    EXPECT_EQ(ps_ports[i], deserialized_ps_ports[i]);
  }
}

TEST(PB_Converter, LimeSamplingParams) {
  LimeSamplingParams lime_sampling_params;
  lime_sampling_params.explain_instance_idx = 5;
  lime_sampling_params.sample_around_instance = false;
  lime_sampling_params.num_total_samples = 100000;
  lime_sampling_params.sampling_method = "gaussian";
  lime_sampling_params.generated_sample_file = "gen_sample_file.txt";

  std::string output_message;
  serialize_lime_sampling_params(lime_sampling_params, output_message);
  LimeSamplingParams output_lime_sampling_params;
  deserialize_lime_sampling_params(output_lime_sampling_params, output_message);

  EXPECT_EQ(lime_sampling_params.explain_instance_idx,
            output_lime_sampling_params.explain_instance_idx);
  EXPECT_EQ(lime_sampling_params.sample_around_instance,
            output_lime_sampling_params.sample_around_instance);
  EXPECT_EQ(lime_sampling_params.num_total_samples,
            output_lime_sampling_params.num_total_samples);
  EXPECT_TRUE(lime_sampling_params.sampling_method ==
              output_lime_sampling_params.sampling_method);
  EXPECT_TRUE(lime_sampling_params.generated_sample_file ==
              output_lime_sampling_params.generated_sample_file);
}

TEST(PB_Converter, LimeCompPredParams) {
  LimeCompPredictionParams lime_comp_pred_params;
  lime_comp_pred_params.original_model_name = "logistic_regression";
  lime_comp_pred_params.original_model_saved_file = "log_reg.txt";
  lime_comp_pred_params.generated_sample_file = "gen_sample_file.txt";
  lime_comp_pred_params.model_type = "classification";
  lime_comp_pred_params.class_num = 2;
  lime_comp_pred_params.computed_prediction_file =
      "computed_prediction_file.txt";

  std::string output_message;
  serialize_lime_comp_pred_params(lime_comp_pred_params, output_message);
  LimeCompPredictionParams output_lime_comp_pred_params;
  deserialize_lime_comp_pred_params(output_lime_comp_pred_params,
                                    output_message);
  EXPECT_TRUE(lime_comp_pred_params.original_model_name ==
              output_lime_comp_pred_params.original_model_name);
  EXPECT_TRUE(lime_comp_pred_params.original_model_saved_file ==
              output_lime_comp_pred_params.original_model_saved_file);
  EXPECT_TRUE(lime_comp_pred_params.generated_sample_file ==
              output_lime_comp_pred_params.generated_sample_file);
  EXPECT_TRUE(lime_comp_pred_params.model_type ==
              output_lime_comp_pred_params.model_type);
  EXPECT_EQ(lime_comp_pred_params.class_num,
            output_lime_comp_pred_params.class_num);
  EXPECT_TRUE(lime_comp_pred_params.computed_prediction_file ==
              output_lime_comp_pred_params.computed_prediction_file);
}

TEST(PB_Converter, LimeCompWeiParams) {
  LimeCompWeightsParams lime_comp_weights_params;
  lime_comp_weights_params.explain_instance_idx = 5;
  lime_comp_weights_params.generated_sample_file = "gen_sample_file.txt";
  lime_comp_weights_params.computed_prediction_file =
      "computed_prediction_file.txt";
  lime_comp_weights_params.is_precompute = false;
  lime_comp_weights_params.num_samples = 5000;
  lime_comp_weights_params.class_num = 2;
  lime_comp_weights_params.distance_metric = "euclidean";
  lime_comp_weights_params.kernel = "exponential";
  lime_comp_weights_params.kernel_width = 0.75;
  lime_comp_weights_params.sample_weights_file = "sample_weights_file.txt";
  lime_comp_weights_params.selected_samples_file = "selected_samples_file.txt";
  lime_comp_weights_params.selected_predictions_file =
      "selected_predictions_file.txt";

  std::string output_message;
  serialize_lime_comp_weights_params(lime_comp_weights_params, output_message);
  LimeCompWeightsParams output_lime_comp_weights_params;
  deserialize_lime_comp_weights_params(output_lime_comp_weights_params,
                                       output_message);

  EXPECT_EQ(lime_comp_weights_params.explain_instance_idx,
            output_lime_comp_weights_params.explain_instance_idx);
  EXPECT_TRUE(lime_comp_weights_params.generated_sample_file ==
              output_lime_comp_weights_params.generated_sample_file);
  EXPECT_TRUE(lime_comp_weights_params.computed_prediction_file ==
              output_lime_comp_weights_params.computed_prediction_file);
  EXPECT_EQ(lime_comp_weights_params.is_precompute,
            output_lime_comp_weights_params.is_precompute);
  EXPECT_EQ(lime_comp_weights_params.num_samples,
            output_lime_comp_weights_params.num_samples);
  EXPECT_EQ(lime_comp_weights_params.class_num,
            output_lime_comp_weights_params.class_num);
  EXPECT_TRUE(lime_comp_weights_params.distance_metric ==
              output_lime_comp_weights_params.distance_metric);
  EXPECT_TRUE(lime_comp_weights_params.kernel ==
              output_lime_comp_weights_params.kernel);
  EXPECT_EQ(lime_comp_weights_params.kernel_width,
            output_lime_comp_weights_params.kernel_width);
  EXPECT_TRUE(lime_comp_weights_params.sample_weights_file ==
              output_lime_comp_weights_params.sample_weights_file);
  EXPECT_TRUE(lime_comp_weights_params.selected_samples_file ==
              output_lime_comp_weights_params.selected_samples_file);
  EXPECT_TRUE(lime_comp_weights_params.selected_predictions_file ==
              output_lime_comp_weights_params.selected_predictions_file);
}

TEST(PB_Converter, LimeFeatureSelectParams) {
  LimeFeatSelParams lime_feat_sel_params;
  lime_feat_sel_params.selected_samples_file = "selected_samples_file.txt";
  lime_feat_sel_params.selected_predictions_file =
      "selected_predictions_file.txt";
  lime_feat_sel_params.sample_weights_file = "sample_weights_file.txt";
  lime_feat_sel_params.num_samples = 5000;
  lime_feat_sel_params.class_num = 2;
  lime_feat_sel_params.class_id = 0;
  lime_feat_sel_params.feature_selection = "pearson";
  lime_feat_sel_params.num_explained_features = 5;
  lime_feat_sel_params.selected_features_file = "selected_features_file.txt";

  std::string output_message;
  serialize_lime_feat_sel_params(lime_feat_sel_params, output_message);
  LimeFeatSelParams output_lime_feat_sel_params;
  deserialize_lime_feat_sel_params(output_lime_feat_sel_params, output_message);

  EXPECT_TRUE(lime_feat_sel_params.selected_samples_file ==
              output_lime_feat_sel_params.selected_samples_file);
  EXPECT_TRUE(lime_feat_sel_params.selected_predictions_file ==
              output_lime_feat_sel_params.selected_predictions_file);
  EXPECT_TRUE(lime_feat_sel_params.sample_weights_file ==
              output_lime_feat_sel_params.sample_weights_file);
  EXPECT_EQ(lime_feat_sel_params.num_samples,
            output_lime_feat_sel_params.num_samples);
  EXPECT_EQ(lime_feat_sel_params.class_num,
            output_lime_feat_sel_params.class_num);
  EXPECT_EQ(lime_feat_sel_params.class_id,
            output_lime_feat_sel_params.class_id);
  EXPECT_TRUE(lime_feat_sel_params.feature_selection ==
              output_lime_feat_sel_params.feature_selection);
  EXPECT_EQ(lime_feat_sel_params.num_explained_features,
            output_lime_feat_sel_params.num_explained_features);
  EXPECT_TRUE(lime_feat_sel_params.selected_features_file ==
              output_lime_feat_sel_params.selected_features_file);
}

TEST(PB_Converter, LimeInterpretParams) {
  LimeInterpretParams lime_interpret_params;
  lime_interpret_params.selected_data_file = "selected_data_file.txt";
  lime_interpret_params.selected_predictions_file =
      "selected_prediction_file.txt";
  lime_interpret_params.sample_weights_file = "sample_weights_file.txt";
  lime_interpret_params.num_samples = 5000;
  lime_interpret_params.class_num = 2;
  lime_interpret_params.class_id = 0;
  lime_interpret_params.interpret_model_name = "linear_regression";
  lime_interpret_params.interpret_model_param = "pb_linear_regression";
  lime_interpret_params.explanation_report = "explanation_report.txt";

  std::string output_message;
  serialize_lime_interpret_params(lime_interpret_params, output_message);
  LimeInterpretParams output_lime_interpret_params;
  deserialize_lime_interpret_params(output_lime_interpret_params,
                                    output_message);

  EXPECT_TRUE(lime_interpret_params.selected_data_file ==
              output_lime_interpret_params.selected_data_file);
  EXPECT_TRUE(lime_interpret_params.selected_predictions_file ==
              output_lime_interpret_params.selected_predictions_file);
  EXPECT_TRUE(lime_interpret_params.sample_weights_file ==
              output_lime_interpret_params.sample_weights_file);
  EXPECT_EQ(lime_interpret_params.num_samples,
            output_lime_interpret_params.num_samples);
  EXPECT_EQ(lime_interpret_params.class_num,
            output_lime_interpret_params.class_num);
  EXPECT_EQ(lime_interpret_params.class_id,
            output_lime_interpret_params.class_id);
  EXPECT_TRUE(lime_interpret_params.interpret_model_name ==
              output_lime_interpret_params.interpret_model_name);
  EXPECT_TRUE(lime_interpret_params.interpret_model_param ==
              output_lime_interpret_params.interpret_model_param);
  EXPECT_TRUE(lime_interpret_params.explanation_report ==
              output_lime_interpret_params.explanation_report);
}
