//
// Created by wuyuncheng on 14/5/21.
//

#include <google/protobuf/io/coded_stream.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <glog/logging.h>
#include <google/protobuf/message_lite.h>
#include "../../include/message/tree.pb.h"

void serialize_encrypted_statistics(int client_id,
    int node_index,
    int split_num,
    int classes_num,
    EncodedNumber* left_sample_nums,
    EncodedNumber* right_sample_nums,
    EncodedNumber ** encrypted_statistics,
    std::string & output_str) {
  com::nus::dbsytem::falcon::v0::EncryptedStatistics pb_encrypted_statistics;

  pb_encrypted_statistics.set_client_id(client_id);
  pb_encrypted_statistics.set_node_index(node_index);
  pb_encrypted_statistics.set_local_split_num(split_num);
  pb_encrypted_statistics.set_classes_num(classes_num);
  if (split_num != 0) {
    // local feature is not used up
    for (int i = 0; i < split_num; i++) {

      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_number_left =
          pb_encrypted_statistics.add_left_sample_nums_of_splits();
      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_number_right =
          pb_encrypted_statistics.add_right_sample_nums_of_splits();

      char * n_str_left_c, * value_str_left_c, * n_str_right_c, * value_str_right_c;
      mpz_t g_n_l, g_value_l;
      mpz_init(g_n_l);
      mpz_init(g_value_l);
      left_sample_nums[i].getter_n(g_n_l);
      left_sample_nums[i].getter_value(g_value_l);
      n_str_left_c = mpz_get_str(NULL, 10, g_n_l);
      value_str_left_c = mpz_get_str(NULL, 10, g_value_l);
      std::string n_str_left(n_str_left_c), value_str_left(value_str_left_c);
      pb_number_left->set_n(n_str_left);
      pb_number_left->set_value(value_str_left);
      pb_number_left->set_exponent(left_sample_nums[i].getter_exponent());
      pb_number_left->set_type(left_sample_nums[i].getter_type());

      mpz_t g_n_r, g_value_r;
      mpz_init(g_n_r);
      mpz_init(g_value_r);
      right_sample_nums[i].getter_n(g_n_r);
      right_sample_nums[i].getter_value(g_value_r);
      n_str_right_c = mpz_get_str(NULL, 10, g_n_r);
      value_str_right_c = mpz_get_str(NULL, 10, g_value_r);
      std::string n_str_right(n_str_right_c), value_str_right(value_str_right_c);
      pb_number_right->set_n(n_str_right);
      pb_number_right->set_value(value_str_right);
      pb_number_right->set_exponent(right_sample_nums[i].getter_exponent());
      pb_number_right->set_type(right_sample_nums[i].getter_type());

      com::nus::dbsytem::falcon::v0::EncryptedStatPerSplit *pb_encrypted_stat_split =
          pb_encrypted_statistics.add_encrypted_stats_of_splits();

      for (int j = 0; j < classes_num * 2; j++) {
        com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_number =
            pb_encrypted_stat_split->add_encrypted_stat();
        char * n_str_c, * value_str_c;
        mpz_t g_n, g_value;
        mpz_init(g_n);
        mpz_init(g_value);
        encrypted_statistics[i][j].getter_n(g_n);
        encrypted_statistics[i][j].getter_value(g_value);
        n_str_c = mpz_get_str(NULL, 10, g_n);
        value_str_c = mpz_get_str(NULL, 10, g_value);
        std::string n_str(n_str_c), value_str(value_str_c);
        pb_number->set_n(n_str);
        pb_number->set_value(value_str);
        pb_number->set_exponent(encrypted_statistics[i][j].getter_exponent());
        pb_number->set_type(encrypted_statistics[i][j].getter_type());
        mpz_clear(g_n);
        mpz_clear(g_value);
        free(n_str_c);
        free(value_str_c);
      }

      mpz_clear(g_n_l);
      mpz_clear(g_value_l);
      mpz_clear(g_n_r);
      mpz_clear(g_value_r);
      free(n_str_left_c);
      free(n_str_right_c);
      free(value_str_left_c);
      free(value_str_right_c);
    }
  }
  pb_encrypted_statistics.SerializeToString(&output_str);
}


void deserialize_encrypted_statistics(int & client_id,
    int & node_index,
    int & split_num,
    int & classes_num,
    EncodedNumber * & left_sample_nums,
    EncodedNumber * & right_sample_nums,
    EncodedNumber ** & encrypted_statistics,
    std::string input_str) {
  com::nus::dbsytem::falcon::v0::EncryptedStatistics deserialized_encrypted_statistics;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_str.c_str(), input_str.length());
  inputStream.SetTotalBytesLimit(1024*1024*1024, 1024*1024*1024);

  if (!deserialized_encrypted_statistics.ParseFromCodedStream(&inputStream)) {
    LOG(ERROR) << "Failed to parse PB_EncryptedStatistics from string";
    return;
  }

  client_id = deserialized_encrypted_statistics.client_id();
  node_index = deserialized_encrypted_statistics.node_index();
  split_num = deserialized_encrypted_statistics.local_split_num();
  classes_num = deserialized_encrypted_statistics.classes_num();

  if (split_num != 0) {
    left_sample_nums = new EncodedNumber[split_num];
    right_sample_nums = new EncodedNumber[split_num];
    encrypted_statistics = new EncodedNumber*[split_num];
    for (int i = 0; i < split_num; i++) {
      encrypted_statistics[i] = new EncodedNumber[2 * classes_num];
    }
    // has encrypted statistics
    for (int i = 0; i < deserialized_encrypted_statistics.left_sample_nums_of_splits_size(); i++) {
      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number =
          deserialized_encrypted_statistics.left_sample_nums_of_splits(i);
      mpz_t g_n, g_value;
      mpz_init(g_n);
      mpz_init(g_value);
      mpz_set_str(g_n, pb_number.n().c_str(), 10);
      mpz_set_str(g_value, pb_number.value().c_str(), 10);
      left_sample_nums[i].setter_n(g_n);
      left_sample_nums[i].setter_value(g_value);
      left_sample_nums[i].setter_exponent(pb_number.exponent());
      EncodedNumberType type = (EncodedNumberType) pb_number.type();
      left_sample_nums[i].setter_type(type);
      mpz_clear(g_n);
      mpz_clear(g_value);
    }

    for (int i = 0; i < deserialized_encrypted_statistics.right_sample_nums_of_splits_size(); i++) {
      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number =
          deserialized_encrypted_statistics.right_sample_nums_of_splits(i);
      mpz_t g_n, g_value;
      mpz_init(g_n);
      mpz_init(g_value);
      mpz_set_str(g_n, pb_number.n().c_str(), 10);
      mpz_set_str(g_value, pb_number.value().c_str(), 10);
      right_sample_nums[i].setter_n(g_n);
      right_sample_nums[i].setter_value(g_value);
      right_sample_nums[i].setter_exponent(pb_number.exponent());
      EncodedNumberType type = (EncodedNumberType) pb_number.type();
      right_sample_nums[i].setter_type(type);
      mpz_clear(g_n);
      mpz_clear(g_value);
    }

    for (int i = 0; i < deserialized_encrypted_statistics.encrypted_stats_of_splits_size(); i++) {
      com::nus::dbsytem::falcon::v0::EncryptedStatPerSplit pb_stat_split =
          deserialized_encrypted_statistics.encrypted_stats_of_splits(i);
      for (int j = 0; j < pb_stat_split.encrypted_stat_size(); j++) {
        com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number = pb_stat_split.encrypted_stat(j);
        mpz_t g_n, g_value;
        mpz_init(g_n);
        mpz_init(g_value);
        mpz_set_str(g_n, pb_number.n().c_str(), 10);
        mpz_set_str(g_value, pb_number.value().c_str(), 10);
        encrypted_statistics[i][j].setter_n(g_n);
        encrypted_statistics[i][j].setter_value(g_value);
        encrypted_statistics[i][j].setter_exponent(pb_number.exponent());
        EncodedNumberType type = (EncodedNumberType) pb_number.type();
        encrypted_statistics[i][j].setter_type(type);
        mpz_clear(g_n);
        mpz_clear(g_value);
      }
    }
  }
}

void serialize_update_info(int source_party_id,
    int best_party_id,
    int best_feature_id,
    int best_split_id,
    EncodedNumber left_branch_impurity,
    EncodedNumber right_branch_impurity,
    EncodedNumber* left_branch_sample_iv,
    EncodedNumber *right_branch_sample_iv,
    int sample_size,
    std::string & output_str) {
  com::nus::dbsytem::falcon::v0::NodeUpdateInfo pb_update_info;
  pb_update_info.set_source_client_id(source_party_id);
  pb_update_info.set_best_client_id(best_party_id);
  pb_update_info.set_best_feature_id(best_feature_id);
  pb_update_info.set_best_split_id(best_split_id);

  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_left_impurity =
      new com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber;
  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_right_impurity =
      new com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber;

  char * n_str_left_c, * value_str_left_c, * n_str_right_c, * value_str_right_c;
  mpz_t g_n_l, g_value_l;
  mpz_init(g_n_l);
  mpz_init(g_value_l);
  left_branch_impurity.getter_n(g_n_l);
  left_branch_impurity.getter_value(g_value_l);
  n_str_left_c = mpz_get_str(NULL, 10, g_n_l);
  value_str_left_c = mpz_get_str(NULL, 10, g_value_l);
  std::string n_str_left(n_str_left_c), value_str_left(value_str_left_c);
  pb_left_impurity->set_n(n_str_left);
  pb_left_impurity->set_value(value_str_left);
  pb_left_impurity->set_exponent(left_branch_impurity.getter_exponent());
  pb_left_impurity->set_type(left_branch_impurity.getter_type());

  mpz_t g_n_r, g_value_r;
  mpz_init(g_n_r);
  mpz_init(g_value_r);
  right_branch_impurity.getter_n(g_n_r);
  right_branch_impurity.getter_value(g_value_r);
  n_str_right_c = mpz_get_str(NULL, 10, g_n_r);
  value_str_right_c = mpz_get_str(NULL, 10, g_value_r);
  std::string n_str_right(n_str_right_c), value_str_right(value_str_right_c);
  pb_right_impurity->set_n(n_str_right);
  pb_right_impurity->set_value(value_str_right);
  pb_right_impurity->set_exponent(right_branch_impurity.getter_exponent());
  pb_right_impurity->set_type(right_branch_impurity.getter_type());

  pb_update_info.set_allocated_left_branch_impurity(pb_left_impurity);
  pb_update_info.set_allocated_right_branch_impurity(pb_right_impurity);

  for (int i = 0; i < sample_size; i++) {
    com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_left_sample_iv =
        pb_update_info.add_left_branch_sample_ivs();
    com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *pb_right_sample_iv =
        pb_update_info.add_right_branch_sample_ivs();

    char * n_str_left_sample_c, * value_str_left_sample_c, * n_str_right_sample_c, * value_str_right_sample_c;
    mpz_t g_n_ll, g_value_ll;
    mpz_init(g_n_ll);
    mpz_init(g_value_ll);
    left_branch_sample_iv[i].getter_n(g_n_ll);
    left_branch_sample_iv[i].getter_value(g_value_ll);
    n_str_left_sample_c = mpz_get_str(NULL, 10, g_n_ll);
    value_str_left_sample_c = mpz_get_str(NULL, 10, g_value_ll);
    std::string n_str_left_sample(n_str_left_sample_c), value_str_left_sample(value_str_left_sample_c);
    pb_left_sample_iv->set_n(n_str_left_sample);
    pb_left_sample_iv->set_value(value_str_left_sample);
    pb_left_sample_iv->set_exponent(left_branch_sample_iv[i].getter_exponent());
    pb_left_sample_iv->set_type(left_branch_sample_iv[i].getter_type());

    mpz_t g_n_rr, g_value_rr;
    mpz_init(g_n_rr);
    mpz_init(g_value_rr);
    right_branch_sample_iv[i].getter_n(g_n_rr);
    right_branch_sample_iv[i].getter_value(g_value_rr);
    n_str_right_sample_c = mpz_get_str(NULL, 10, g_n_rr);
    value_str_right_sample_c = mpz_get_str(NULL, 10, g_value_rr);
    std::string n_str_right_sample(n_str_right_sample_c), value_str_right_sample(value_str_right_sample_c);
    pb_right_sample_iv->set_n(n_str_right_sample);
    pb_right_sample_iv->set_value(value_str_right_sample);
    pb_right_sample_iv->set_exponent(right_branch_sample_iv[i].getter_exponent());
    pb_right_sample_iv->set_type(right_branch_sample_iv[i].getter_type());

    mpz_clear(g_n_ll);
    mpz_clear(g_value_ll);
    mpz_clear(g_n_rr);
    mpz_clear(g_value_rr);
    free(n_str_left_sample_c);
    free(value_str_left_sample_c);
    free(n_str_right_sample_c);
    free(value_str_right_sample_c);
  }

  pb_update_info.SerializeToString(&output_str);

  mpz_clear(g_n_l);
  mpz_clear(g_value_l);
  mpz_clear(g_n_r);
  mpz_clear(g_value_r);
  free(n_str_left_c);
  free(value_str_left_c);
  free(n_str_right_c);
  free(value_str_right_c);
}


void deserialize_update_info(int & source_client_id,
    int & best_client_id,
    int & best_feature_id,
    int & best_split_id,
    EncodedNumber & left_branch_impurity,
    EncodedNumber & right_branch_impurity,
    EncodedNumber* & left_branch_sample_iv,
    EncodedNumber* & right_branch_sample_iv,
    std::string input_str) {
  com::nus::dbsytem::falcon::v0::NodeUpdateInfo deserialized_update_info;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_str.c_str(), input_str.length());
  inputStream.SetTotalBytesLimit(1024*1024*1024, 1024*1024*1024);

  if (!deserialized_update_info.ParseFromCodedStream(&inputStream)) {
    LOG(ERROR) << "Failed to parse PB_UpdateInfo from string";
    return;
  }

  source_client_id = deserialized_update_info.source_client_id();
  best_client_id = deserialized_update_info.best_client_id();
  best_feature_id = deserialized_update_info.best_feature_id();
  best_split_id = deserialized_update_info.best_split_id();

  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number_left =
      deserialized_update_info.left_branch_impurity();
  com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number_right =
      deserialized_update_info.right_branch_impurity();

  mpz_t g_n_l, g_value_l;
  mpz_init(g_n_l);
  mpz_init(g_value_l);
  mpz_set_str(g_n_l, pb_number_left.n().c_str(), 10);
  mpz_set_str(g_value_l, pb_number_left.value().c_str(), 10);
  left_branch_impurity.setter_n(g_n_l);
  left_branch_impurity.setter_value(g_value_l);
  left_branch_impurity.setter_exponent(pb_number_left.exponent());
  EncodedNumberType type_l = (EncodedNumberType) pb_number_left.type();
  left_branch_impurity.setter_type(type_l);

  mpz_t g_n_r, g_value_r;
  mpz_init(g_n_r);
  mpz_init(g_value_r);
  mpz_set_str(g_n_r, pb_number_right.n().c_str(), 10);
  mpz_set_str(g_value_r, pb_number_right.value().c_str(), 10);
  right_branch_impurity.setter_n(g_n_r);
  right_branch_impurity.setter_value(g_value_r);
  right_branch_impurity.setter_exponent(pb_number_right.exponent());
  EncodedNumberType type_r = (EncodedNumberType) pb_number_right.type();
  right_branch_impurity.setter_type(type_r);

  int sample_size = deserialized_update_info.left_branch_sample_ivs_size();
  left_branch_sample_iv = new EncodedNumber[sample_size];
  right_branch_sample_iv = new EncodedNumber[sample_size];

  for (int i = 0; i < sample_size; i++) {
    com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number_left_iv =
        deserialized_update_info.left_branch_sample_ivs(i);
    com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber pb_number_right_iv =
        deserialized_update_info.right_branch_sample_ivs(i);

    mpz_t g_n_ll, g_value_ll;
    mpz_init(g_n_ll);
    mpz_init(g_value_ll);
    mpz_set_str(g_n_ll, pb_number_left_iv.n().c_str(), 10);
    mpz_set_str(g_value_ll, pb_number_left_iv.value().c_str(), 10);
    left_branch_sample_iv[i].setter_n(g_n_ll);
    left_branch_sample_iv[i].setter_value(g_value_ll);
    left_branch_sample_iv[i].setter_exponent(pb_number_left_iv.exponent());
    EncodedNumberType type_ll = (EncodedNumberType) pb_number_left_iv.type();
    left_branch_sample_iv[i].setter_type(type_ll);

    mpz_t g_n_rr, g_value_rr;
    mpz_init(g_n_rr);
    mpz_init(g_value_rr);
    mpz_set_str(g_n_rr, pb_number_right_iv.n().c_str(), 10);
    mpz_set_str(g_value_rr, pb_number_right_iv.value().c_str(), 10);
    right_branch_sample_iv[i].setter_n(g_n_rr);
    right_branch_sample_iv[i].setter_value(g_value_rr);
    right_branch_sample_iv[i].setter_exponent(pb_number_right_iv.exponent());
    EncodedNumberType type_rr = (EncodedNumberType) pb_number_right_iv.type();
    right_branch_sample_iv[i].setter_type(type_rr);
    mpz_clear(g_n_ll);
    mpz_clear(g_value_ll);
    mpz_clear(g_n_rr);
    mpz_clear(g_value_rr);
  }
  mpz_clear(g_n_l);
  mpz_clear(g_value_l);
  mpz_clear(g_n_r);
  mpz_clear(g_value_r);
}


void serialize_split_info(int global_split_num,
    std::vector<int> client_split_nums,
    std::string & output_str) {
  com::nus::dbsytem::falcon::v0::SplitInfo pb_split_info;
  pb_split_info.set_global_split_num(global_split_num);
  for (int i = 0; i < client_split_nums.size(); i++) {
    pb_split_info.add_split_num_vec(client_split_nums[i]);
  }
  pb_split_info.SerializeToString(&output_str);
}


void deserialize_split_info(int & global_split_num,
    std::vector<int> & client_split_nums,
    std::string input_str) {
  com::nus::dbsytem::falcon::v0::SplitInfo deserialized_split_info;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_str.c_str(), input_str.length());
  inputStream.SetTotalBytesLimit(1024*1024*1024, 1024*1024*1024);
  if (!deserialized_split_info.ParseFromCodedStream(&inputStream)) {
    LOG(ERROR) << "Failed to parse PB_SplitInfo from string";
    return;
  }
  global_split_num = deserialized_split_info.global_split_num();
  for (int i = 0; i < deserialized_split_info.split_num_vec_size(); i++) {
    client_split_nums.push_back(deserialized_split_info.split_num_vec(i));
  }
}