//
// Created by wuyuncheng on 13/8/20.
//

#include <string>
#include <iostream>

#include <gtest/gtest.h>
#include <falcon/utils/pb_converter/model_converter.h>
#include <falcon/utils/pb_converter/phe_keys_converter.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/alg_params_converter.h>
#include <falcon/utils/pb_converter/network_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>

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
  EncodedNumber* encoded_number_array = new EncodedNumber[3];
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
  EXPECT_EQ(dt_params.min_samples_split, deserialized_dt_params.min_samples_split);
  EXPECT_EQ(dt_params.min_samples_leaf, deserialized_dt_params.min_samples_leaf);
  EXPECT_EQ(dt_params.max_leaf_nodes, deserialized_dt_params.max_leaf_nodes);
  EXPECT_EQ(dt_params.min_impurity_decrease, deserialized_dt_params.min_impurity_decrease);
  EXPECT_EQ(dt_params.min_impurity_split, deserialized_dt_params.min_impurity_split);
  EXPECT_EQ(dt_params.dp_budget, deserialized_dt_params.dp_budget);
  EXPECT_TRUE(dt_params.tree_type == deserialized_dt_params.tree_type);
  EXPECT_TRUE(dt_params.criterion == deserialized_dt_params.criterion);
  EXPECT_TRUE(dt_params.split_strategy == deserialized_dt_params.split_strategy);
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
  EXPECT_EQ(rf_params.dt_param.class_num, deserialized_rf_params.dt_param.class_num);
  EXPECT_EQ(rf_params.dt_param.max_depth, deserialized_rf_params.dt_param.max_depth);
  EXPECT_EQ(rf_params.dt_param.max_bins, deserialized_rf_params.dt_param.max_bins);
  EXPECT_EQ(rf_params.dt_param.min_samples_split, deserialized_rf_params.dt_param.min_samples_split);
  EXPECT_EQ(rf_params.dt_param.min_samples_leaf, deserialized_rf_params.dt_param.min_samples_leaf);
  EXPECT_EQ(rf_params.dt_param.max_leaf_nodes, deserialized_rf_params.dt_param.max_leaf_nodes);
  EXPECT_EQ(rf_params.dt_param.min_impurity_decrease, deserialized_rf_params.dt_param.min_impurity_decrease);
  EXPECT_EQ(rf_params.dt_param.min_impurity_split, deserialized_rf_params.dt_param.min_impurity_split);
  EXPECT_EQ(rf_params.dt_param.dp_budget, deserialized_rf_params.dt_param.dp_budget);
  EXPECT_TRUE(rf_params.dt_param.tree_type == deserialized_rf_params.dt_param.tree_type);
  EXPECT_TRUE(rf_params.dt_param.criterion == deserialized_rf_params.dt_param.criterion);
  EXPECT_TRUE(rf_params.dt_param.split_strategy == deserialized_rf_params.dt_param.split_strategy);
}

TEST(PB_Converter, TreeEncryptedStatistics) {
  int client_id = 0;
  int node_index = 1;
  int split_num = 3;
  int classes_num = 2;
  EncodedNumber *left_sample_nums = new EncodedNumber[3];
  EncodedNumber *right_sample_nums = new EncodedNumber[3];
  EncodedNumber ** encrypted_statistics = new EncodedNumber*[3];
  for (int i = 0; i < 3; i++) {
    encrypted_statistics[i] = new EncodedNumber[2*2];
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
    for (int j = 0; j < 2*2; j++) {
      encrypted_statistics[i][j].setter_n(v_n);
      encrypted_statistics[i][j].setter_value(v_value);
      encrypted_statistics[i][j].setter_exponent(v_exponent);
      encrypted_statistics[i][j].setter_type(v_type);
    }
  }

  std::string output_message;
  serialize_encrypted_statistics(client_id, node_index,
      split_num, classes_num,
      left_sample_nums, right_sample_nums,
      encrypted_statistics, output_message);

  int deserialized_client_id = 0;
  int deserialized_node_index = 1;
  int deserialized_split_num = 3;
  int deserialized_classes_num = 2;
  EncodedNumber *deserialized_left_sample_nums;
  EncodedNumber *deserialized_right_sample_nums;
  EncodedNumber ** deserialized_encrypted_statistics;

  deserialize_encrypted_statistics(deserialized_client_id, deserialized_node_index,
      deserialized_split_num, deserialized_classes_num,
      deserialized_left_sample_nums, deserialized_right_sample_nums,
      deserialized_encrypted_statistics, output_message);

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
    for (int j = 0; j < 2*2; j++) {
      mpz_t deserialized_n, deserialized_value;
      mpz_init(deserialized_n);
      mpz_init(deserialized_value);
      deserialized_encrypted_statistics[i][j].getter_n(deserialized_n);
      deserialized_encrypted_statistics[i][j].getter_value(deserialized_value);
      int n_cmp = mpz_cmp(v_n, deserialized_n);
      int value_cmp = mpz_cmp(v_value, deserialized_value);
      EXPECT_EQ(0, n_cmp);
      EXPECT_EQ(0, value_cmp);
      EXPECT_EQ(v_exponent, deserialized_encrypted_statistics[i][j].getter_exponent());
      EXPECT_EQ(v_type, deserialized_encrypted_statistics[i][j].getter_type());
      mpz_clear(deserialized_n);
      mpz_clear(deserialized_value);
    }
  }

  mpz_clear(v_n);
  mpz_clear(v_value);
  delete [] left_sample_nums;
  delete [] right_sample_nums;
  delete [] deserialized_left_sample_nums;
  delete [] deserialized_right_sample_nums;
  for (int i = 0; i < 3; i++) {
    delete [] encrypted_statistics[i];
    delete [] deserialized_encrypted_statistics[i];
  }
  delete [] encrypted_statistics;
  delete [] deserialized_encrypted_statistics;
}

TEST(PB_Converter, TreeUpdateInfo) {
  int source_party_id = 0;
  int best_party_id = 0;
  int best_feature_id = 1;
  int best_split_id = 2;
  EncodedNumber left_impurity, right_impurity;
  EncodedNumber *left_sample_iv = new EncodedNumber[3];
  EncodedNumber *right_sample_iv = new EncodedNumber[3];
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
  serialize_update_info(source_party_id, best_party_id,
      best_feature_id, best_split_id, left_impurity, right_impurity,
      left_sample_iv, right_sample_iv, 3, out_message);
  int deserialized_source_party_id, deserialized_best_party_id;
  int deserialized_best_feature_id, deserialized_best_split_id;
  EncodedNumber deserialized_left_impurity, deserialized_right_impurity;
  EncodedNumber* deserialized_left_sample_iv = new EncodedNumber[3];
  EncodedNumber* deserialized_right_sample_iv = new EncodedNumber[3];
  deserialize_update_info(deserialized_source_party_id,
      deserialized_best_party_id, deserialized_best_feature_id,
      deserialized_best_split_id, deserialized_left_impurity,
      deserialized_right_impurity, deserialized_left_sample_iv,
      deserialized_right_sample_iv, out_message);

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
  delete [] left_sample_iv;
  delete [] right_sample_iv;
  delete [] deserialized_left_sample_iv;
  delete [] deserialized_right_sample_iv;
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
  deserialize_split_info(deserialized_global_split_num, deserialized_client_split_nums, output_message);

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
    EXPECT_EQ(deserialized_tree.nodes[i].is_self_feature, tree.nodes[i].is_self_feature);
    EXPECT_EQ(deserialized_tree.nodes[i].best_party_id, tree.nodes[i].best_party_id);
    EXPECT_EQ(deserialized_tree.nodes[i].best_feature_id, tree.nodes[i].best_feature_id);
    EXPECT_EQ(deserialized_tree.nodes[i].best_split_id, tree.nodes[i].best_split_id);
    EXPECT_EQ(deserialized_tree.nodes[i].split_threshold, tree.nodes[i].split_threshold);
    EXPECT_EQ(deserialized_tree.nodes[i].node_sample_num, tree.nodes[i].node_sample_num);
    EXPECT_EQ(deserialized_tree.nodes[i].left_child, tree.nodes[i].left_child);
    EXPECT_EQ(deserialized_tree.nodes[i].right_child, tree.nodes[i].right_child);
    for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
      EXPECT_EQ(deserialized_tree.nodes[i].node_sample_distribution[j], tree.nodes[i].node_sample_distribution[j]);
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
  std::vector<TreeModel> trees;
  int n_estimator = 3;
  mpz_t v_n;
  mpz_t v_value;
  mpz_init(v_n);
  mpz_init(v_value);
  mpz_set_str(v_n, "100000000000000", PHE_STR_BASE);
  mpz_set_str(v_value, "100", PHE_STR_BASE);
  int v_exponent = -8;
  EncodedNumberType v_type = Ciphertext;
  for (int t = 0; t < n_estimator; t++) {
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
    trees.push_back(tree);
  }

  std::string out_message;
  serialize_random_forest_model(trees, n_estimator,out_message);

  std::vector<TreeModel> deserialized_trees;
  int deserialized_n_estimator;
  deserialize_random_forest_model(deserialized_trees, deserialized_n_estimator, out_message);
  EXPECT_EQ(n_estimator, deserialized_n_estimator);
  for (int t = 0; t < n_estimator; t++) {
    TreeModel tree = deserialized_trees[t];
    EXPECT_EQ(deserialized_trees[t].type, tree.type);
    EXPECT_EQ(deserialized_trees[t].class_num, tree.class_num);
    EXPECT_EQ(deserialized_trees[t].max_depth, tree.max_depth);
    EXPECT_EQ(deserialized_trees[t].internal_node_num, tree.internal_node_num);
    EXPECT_EQ(deserialized_trees[t].total_node_num, tree.total_node_num);
    EXPECT_EQ(deserialized_trees[t].capacity, tree.capacity);
    for (int i = 0; i < tree.capacity; i++) {
      EXPECT_EQ(deserialized_trees[t].nodes[i].node_type, tree.nodes[i].node_type);
      EXPECT_EQ(deserialized_trees[t].nodes[i].depth, tree.nodes[i].depth);
      EXPECT_EQ(deserialized_trees[t].nodes[i].is_self_feature, tree.nodes[i].is_self_feature);
      EXPECT_EQ(deserialized_trees[t].nodes[i].best_party_id, tree.nodes[i].best_party_id);
      EXPECT_EQ(deserialized_trees[t].nodes[i].best_feature_id, tree.nodes[i].best_feature_id);
      EXPECT_EQ(deserialized_trees[t].nodes[i].best_split_id, tree.nodes[i].best_split_id);
      EXPECT_EQ(deserialized_trees[t].nodes[i].split_threshold, tree.nodes[i].split_threshold);
      EXPECT_EQ(deserialized_trees[t].nodes[i].node_sample_num, tree.nodes[i].node_sample_num);
      EXPECT_EQ(deserialized_trees[t].nodes[i].left_child, tree.nodes[i].left_child);
      EXPECT_EQ(deserialized_trees[t].nodes[i].right_child, tree.nodes[i].right_child);
      for (int j = 0; j < tree.nodes[i].node_sample_distribution.size(); j++) {
        EXPECT_EQ(deserialized_trees[t].nodes[i].node_sample_distribution[j],
            tree.nodes[i].node_sample_distribution[j]);
      }

      EncodedNumber deserialized_impurity = deserialized_trees[t].nodes[i].impurity;
      EncodedNumber deserialized_label = deserialized_trees[t].nodes[i].label;
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


