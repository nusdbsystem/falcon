//
// Created by naili on 22/6/22.
//

#include "falcon/algorithm/vertical/preprocessing/pearson_correlation.h"
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/operator/conversion/op_conv.h>


EncodedNumber PearsonCorrelation::calculate_score(
    const Party &party,
    std::vector<double > plain_samples,
    EncodedNumber* encrypted_labels,
    int sample_size,
    EncodedNumber* encrypted_weight) {

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // if no feature weights, default all weight value to be 1
  if (encrypted_weight == NULL){
    encrypted_weight = new EncodedNumber[sample_size];
    for (int i = 0; i < sample_size; i++) {
      encrypted_weight[i].set_double(phe_pub_key->n[0], 1, PHE_FIXED_POINT_PRECISION);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random, encrypted_weight[i], encrypted_weight[i]);
    }
  }

  EncodedNumber meanX = this->calculate_weighted_mean( party, encrypted_weight, plain_samples, sample_size );
  EncodedNumber meanY = this->calculate_weighted_mean( party, encrypted_weight, encrypted_labels, sample_size );

  EncodedNumber Sxy = this->calculate_weight_covariance( party,
                                                         encrypted_weight, plain_samples,
                                                         meanX, encrypted_labels,
                                                         meanY, sample_size );


  // encrypted the feature vector.
  auto* encrypted_feature_vector = new EncodedNumber[plain_samples.size()];
  for (int i = 0; i < plain_samples.size(); i++ ){
    encrypted_feature_vector[i].set_double(phe_pub_key->n[0],
                                           plain_samples[i],
                                           PHE_FIXED_POINT_PRECISION);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                       encrypted_feature_vector[i], encrypted_feature_vector[i]);
  }

  EncodedNumber Sx = this->calculate_weighted_variance( party, encrypted_weight, encrypted_feature_vector, meanX, sample_size );
  EncodedNumber Sy = this->calculate_weighted_variance( party, encrypted_weight, encrypted_labels, meanY, sample_size );

//  float score = Sxy / sqrt( Sx * Sy);

  return Sxy;
}


EncodedNumber PearsonCorrelation::calculate_weighted_mean(
    const Party &party,
    EncodedNumber* encrypted_weight,
    std::vector<double > plain_samples,
    int sample_size){

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  EncodedNumber numerator;

  EncodedNumber denominator;
  denominator.set_integer(phe_pub_key->n[0], 0);

  // compute homomorphic inner product between feature j and weights
  djcs_t_aux_inner_product(phe_pub_key, party.phe_random, numerator,
                           encrypted_weight, plain_samples,
                           sample_size);

  for ( int i = 0; i < sample_size; i++ ){
    djcs_t_aux_ee_add(phe_pub_key, denominator, encrypted_weight[i], encrypted_weight[i]);
  }

  //  return numerator/denominator;
  return numerator;
}



EncodedNumber PearsonCorrelation::calculate_weighted_mean(
    const Party &party,
    EncodedNumber* encrypted_weight,
    EncodedNumber* encrypted_samples,
    int sample_size){

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // init numerator and denominator
  EncodedNumber numerator;
  numerator.set_integer(phe_pub_key->n[0], 0);

  EncodedNumber denominator;
  denominator.set_integer(phe_pub_key->n[0], 0);

  // calculate weight[i] * x[i] =>  x
  ciphers_ele_wise_multi(party, encrypted_samples, encrypted_samples, encrypted_weight, sample_size, party.party_id);

  // sum all x
  for ( int i = 0; i < sample_size; i++ ){
    djcs_t_aux_ee_add(phe_pub_key, numerator, encrypted_samples[i], encrypted_samples[i]);
  }

  // add weight to weight[0]
  for ( int i = 0; i < sample_size; i++ ){
    djcs_t_aux_ee_add(phe_pub_key, denominator, encrypted_weight[i], encrypted_weight[i]);
  }

  //  return numerator/denominator;
  return numerator;
}


EncodedNumber PearsonCorrelation::calculate_weighted_variance(
    const Party &party,
    EncodedNumber* encrypted_weight,
    EncodedNumber* encrypted_feature_vector,
    EncodedNumber mean,
    int sample_size){

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // compute [0-meanX] => mean
  int neg_one = -1;
  EncodedNumber enc_neg_one;
  enc_neg_one.set_integer(phe_pub_key->n[0], neg_one);
  djcs_t_aux_ep_mul(phe_pub_key, mean, mean, enc_neg_one);


  for (int i = 0; i < sample_size; i++) {
    // compute phe addition (x[i] - meanX) => encrypted_feature_vector
    djcs_t_aux_ee_add(phe_pub_key,
                      encrypted_feature_vector[i],
                      encrypted_feature_vector[i],
                      mean);

    // compute weight[i] * (x[i] - meanX) => encrypted_feature_vector
    djcs_t_aux_ep_mul(phe_pub_key,
                      encrypted_feature_vector[i],
                      encrypted_feature_vector[i],
                      encrypted_weight[i] );
  }

  // add encrypted_feature_vector and weight

  EncodedNumber numerator;
  numerator.set_integer(phe_pub_key->n[0], 0);

  EncodedNumber denominator;
  denominator.set_integer(phe_pub_key->n[0], 0);

  for (int i = 0; i < sample_size; i++) {

    djcs_t_aux_ee_add(phe_pub_key, numerator, encrypted_feature_vector[i], encrypted_feature_vector[i]);
    djcs_t_aux_ee_add(phe_pub_key, denominator, encrypted_weight[i], encrypted_weight[i]);
  }

  //  return numerator/denominator;
  return numerator;
}


EncodedNumber PearsonCorrelation::calculate_weight_covariance(
    const Party &party,
    EncodedNumber* encrypted_weight,
    std::vector<double > plain_samples,
    EncodedNumber mean_sample,
    EncodedNumber* encrypted_labels,
    EncodedNumber mean_label,
    int sample_size){

  // retrieve phe pub key and phe random
  djcs_t_public_key* phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // compute [0-meanX] => mean
  int neg_one = -1;
  EncodedNumber enc_neg_one;
  enc_neg_one.set_integer(phe_pub_key->n[0], neg_one);
  djcs_t_aux_ep_mul(phe_pub_key, mean_sample, mean_sample, enc_neg_one);
  djcs_t_aux_ep_mul(phe_pub_key, mean_label, mean_label, enc_neg_one);

  // encrypted the feature vector.
  auto* encrypted_feature_vector = new EncodedNumber[plain_samples.size()];
  for (int i = 0; i < plain_samples.size(); i++ ){
    encrypted_feature_vector[i].set_double(phe_pub_key->n[0],
                                           plain_samples[i],
                                           PHE_FIXED_POINT_PRECISION);
    djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                       encrypted_feature_vector[i], encrypted_feature_vector[i]);
  }

  for (int i = 0; i < sample_size; i++) {
    // compute phe addition (x[i] - meanX) => encrypted_feature_vector
    djcs_t_aux_ee_add(phe_pub_key,
                      encrypted_feature_vector[i],
                      encrypted_feature_vector[i],
                      mean_sample);

    // compute phe addition (y[i] - meanX) => encrypted_feature_vector
    djcs_t_aux_ee_add(phe_pub_key,
                      encrypted_labels[i],
                      encrypted_labels[i],
                      mean_label);

    // compute weight[i] * (x[i] - meanX) => encrypted_feature_vector
    djcs_t_aux_ep_mul(phe_pub_key,
                      encrypted_feature_vector[i],
                      encrypted_feature_vector[i],
                      encrypted_weight[i] );

    // compute weight[i] * (x[i] - meanX) * ( y[i] - meanY ) => encrypted_feature_vector
    djcs_t_aux_ep_mul(phe_pub_key,
                      encrypted_feature_vector[i],
                      encrypted_feature_vector[i],
                      encrypted_labels[i] );

  }

  // add encrypted_feature_vector and weight

  EncodedNumber numerator;
  numerator.set_integer(phe_pub_key->n[0], 0);

  EncodedNumber denominator;
  denominator.set_integer(phe_pub_key->n[0], 0);

  for (int i = 0; i < sample_size; i++) {

    djcs_t_aux_ee_add(phe_pub_key, numerator, encrypted_feature_vector[i], encrypted_feature_vector[i]);
    djcs_t_aux_ee_add(phe_pub_key, denominator, encrypted_weight[i], encrypted_weight[i]);
  }

  return numerator;

}




