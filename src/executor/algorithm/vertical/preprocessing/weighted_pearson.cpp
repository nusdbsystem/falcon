//
// Created by naili on 21/8/22.
//

#include "falcon/algorithm/vertical/preprocessing/weighted_pearson.h"
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>


std::vector<double> WeightedPearson::get_feature_importance(Party party,
                                                     int class_id,
                                                     const vector<std::vector<double>> &train_data,
                                                     EncodedNumber *predictions,
                                                     const vector<double> &sss_sample_weights,
                                                     const string &ps_network_str,
                                                     int is_distributed,
                                                     int distributed_role,
                                                     int worker_id) {
  // init local explanation result
  std::vector<double> local_explanations;

  // get datasets
  std::vector<std::vector<double>> used_train_data = train_data;
  int weight_size = (int) used_train_data[0].size();
  party.setter_feature_num(weight_size);
  log_info("start weighted pearson");

  local_explanations = get_correlation();

  return local_explanations;
}


std::vector<double> WeightedPearson::get_correlation(Party party,
                                                     int class_id,
                                                     const vector<std::vector<double>> &train_data,
                                                     EncodedNumber *predictions,
                                                     const vector<double> &sss_sample_weights) {
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();

  // 1. jointly convert <w> into ciphertext [w], and saved in active party
  int num_instance = train_data.size();
  auto* cipher_instance_weight = new EncodedNumber[num_instance];
  secret_shares_to_ciphers(party,
                           cipher_instance_weight,
                           sss_sample_weights,
                           num_instance,
                           ACTIVE_PARTY_ID,
                           PHE_FIXED_POINT_PRECISION);

  // 2. activate party get the sum and convert it to secret shares, and each party hold one share
  log_info("step 2, getting the sum of weight");
  EncodedNumber *sum_w = &cipher_instance_weight[0];
  for (int i = 1; i < num_instance; i++ ){
    djcs_t_aux_ee_add(phe_pub_key, *sum_w, *sum_w, cipher_instance_weight[i]);
  }

  std::vector<double> share_sum_w;
  ciphers_to_secret_shares(party, sum_w, share_sum_w, num_instance ,ACTIVE_PARTY_ID, PHE_FIXED_POINT_PRECISION);

  if (party.party_type == falcon::ACTIVE_PARTY) {
    // 3. active party encrypted local label L to [L]
    log_info("step 2, encrypted label to [y]");

    auto** encrypted_label = new EncodedNumber*[num_instance];
    for (int i = 0; i < num_instance; i++) {
      encrypted_label[i] = new EncodedNumber[1];
    }
    for (int i = 0; i < num_instance; i++){
      encrypted_label[i][0].set_integer(phe_pub_key->n[0], class_id);
    }

    auto** ciphers_shares_mul_res = new EncodedNumber*[1];
    for (int i = 0; i < 1; i++) {
      ciphers_shares_mul_res[i] = new EncodedNumber[1];
    }

    std::vector<std::vector<double>> two_d_weights;
    two_d_weights.push_back(sss_sample_weights);
    // 4. active party calculate [L*W] and convert it to secret share
    cipher_shares_mat_mul(party,
                          two_d_weights,
                          encrypted_label, 1, num_instance, num_instance, 1,
                          ciphers_shares_mul_res);
  }







  djcs_t_free_public_key(phe_pub_key);

  return std::vector<double>();
}


