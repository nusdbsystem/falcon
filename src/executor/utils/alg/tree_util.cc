//
// Created by root on 3/11/22.
//

#include <falcon/utils/alg/tree_util.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/alg/debug_util.h>
#include <numeric>

double root_impurity(const std::vector<double>& labels, falcon::TreeType tree_type, int class_num) {
  // check classification or regression
  // if classification, impurity = 1 - \sum_{c=1}^{class_num} (p_c)^2
  // if regression, impurity = (1/n) \sum_{i=1}^n (y_i)^2 - ((1/n) \sum_{i=1}^n y_i)^2
  double impurity = 0.0;
  int n = (int) labels.size();
  log_info("[tree_util.root_impurity] n = " + std::to_string(n));
  // if classification
  if (tree_type == falcon::CLASSIFICATION) {
    // count samples for each class
    std::vector<double> class_bins;
    class_bins.reserve(class_num);
    for (int i = 0; i < class_num; i++) {
      class_bins.push_back(0.0);
    }
    for (int i = 0; i < n; i++) {
      int c = (int) labels[i];
      class_bins[c] += 1.0;
    }
    // compute probabilities and impurity
    impurity += 1.0;
    for (int c = 0; c < class_num; c++) {
      class_bins[c] = class_bins[c] / ((double) n);
      impurity = impurity - class_bins[c] * class_bins[c];
    }
  }

  // if regression
  if (tree_type == falcon::REGRESSION) {
    // compute labels square
    std::vector<double> labels_square;
    labels_square.reserve(n);
    for (int i = 0; i < n; i++) {
      labels_square.push_back(labels[i] * labels[i]);
    }
    // compute summation
    double label_sum = 0.0, label_square_sum = 0.0;
    for (int i = 0; i < n; i++) {
      label_sum += labels[i];
      label_square_sum += labels_square[i];
    }
    // compute impurity
    impurity = (label_square_sum / n) - (label_sum / n) * (label_sum / n);
  }

  return impurity;
}


double lime_reg_tree_root_impurity(Party& party, EncodedNumber& enc_root_impurity,
                                   bool use_encrypted_labels,
                                   EncodedNumber* weighted_encrypted_true_labels, int size, int class_num,
                                   bool use_sample_weights, const std::vector<double> &sss_sample_weights) {
  log_info("[lime_reg_tree_root_impurity] compute encrypted root impurity");
  // here the encrypted labels are weighted label and label square, resp.
  // 1. compute weight_sum = \sum{encrypted_weights}
  // 2. compute weighted_label_sum and weighted_label_square_sum
  // 3. decrypt and compute impurity = (label_square_sum / weight_sum) - (label_sum / weight_sum) * (label_sum / weight_sum)

#ifdef DEBUG
  debug_cipher_array<double>(party, weighted_encrypted_true_labels, 10, ACTIVE_PARTY_ID, true, 10);
#endif

  auto* enc_label_sum = new EncodedNumber[2];
  auto* dec_label_sum = new EncodedNumber[2];

  // retrieve phe pub key
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  if (party.party_type == falcon::ACTIVE_PARTY) {
    int enc_label_prec = std::abs(weighted_encrypted_true_labels[0].getter_exponent());
    log_info("[lime_reg_tree_root_impurity] enc_label_prec = " + std::to_string(enc_label_prec));
    log_info("[lime_reg_tree_root_impurity] size = " + std::to_string(size));

    enc_label_sum[0].set_double(phe_pub_key->n[0], 0.0, enc_label_prec);
    enc_label_sum[1].set_double(phe_pub_key->n[0], 0.0, enc_label_prec);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, enc_label_sum[0], enc_label_sum[0]);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, enc_label_sum[1], enc_label_sum[1]);

    // compute encrypted sum
    for (int i = 0; i < size; i++) {
      djcs_t_aux_ee_add(phe_pub_key, enc_label_sum[0], enc_label_sum[0], weighted_encrypted_true_labels[i]);
      djcs_t_aux_ee_add(phe_pub_key, enc_label_sum[1], enc_label_sum[1], weighted_encrypted_true_labels[i + size]);
    }
  }
  broadcast_encoded_number_array(party, enc_label_sum, 2, ACTIVE_PARTY_ID);
  // decrypt the sums
  collaborative_decrypt(party, enc_label_sum, dec_label_sum, 2, ACTIVE_PARTY_ID);

  std::vector<double> sample_weights = display_shares_vector(party, sss_sample_weights);

  double root_impurity;
  auto* encrypted_root_impurity = new EncodedNumber[1];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // compute impurity
    double weight_sum, label_sum, label_square_sum;
    weight_sum = std::accumulate(sample_weights.begin(), sample_weights.end(), 0.0);
    dec_label_sum[0].decode(label_sum);
    dec_label_sum[1].decode(label_square_sum);
    log_info("[lime_reg_tree_root_impurity] weight_sum = " + std::to_string(weight_sum));
    log_info("[lime_reg_tree_root_impurity] label_sum = " + std::to_string(label_sum));
    log_info("[lime_reg_tree_root_impurity] label_square_sum = " + std::to_string(label_square_sum));
    root_impurity = (label_square_sum / weight_sum) - (label_sum / weight_sum) * (label_sum / weight_sum);
    // encrypt the root impurity
    encrypted_root_impurity[0].set_double(phe_pub_key->n[0], root_impurity, PHE_FIXED_POINT_PRECISION);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_root_impurity[0], encrypted_root_impurity[0]);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, std::to_string(root_impurity));
      }
    }
  } else {
    std::string root_impurity_str;
    party.recv_long_message(ACTIVE_PARTY_ID, root_impurity_str);
    root_impurity = std::stod(root_impurity_str);
  }
  log_info("[lime_reg_tree_root_impurity] root_impurity = " + std::to_string(root_impurity));

  broadcast_encoded_number_array(party, encrypted_root_impurity, 1, ACTIVE_PARTY_ID);
  enc_root_impurity = encrypted_root_impurity[0];

  djcs_t_free_public_key(phe_pub_key);
  delete [] enc_label_sum;
  delete [] dec_label_sum;
  delete [] encrypted_root_impurity;
  return root_impurity;
}


std::vector<double> rf_pred2prob(int class_num, const std::vector<double>& pred) {
  std::vector<double> prob;
  prob.reserve(class_num);
  for (int i = 0; i < class_num; i++) {
    prob.push_back(0.0);
  }
  for (double p : pred) {
    int class_id = (int) p;
    prob[class_id] += 1.0;
  }
  double sum = std::accumulate(prob.begin(), prob.end(), 0.0);
  for (int i = 0; i < class_num; i++) {
    prob[i] = prob[i] / sum;
  }
  return prob;
}