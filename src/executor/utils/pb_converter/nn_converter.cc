//
// Created by root on 5/25/22.
//

#include <falcon/utils/pb_converter/nn_converter.h>

#include <google/protobuf/io/coded_stream.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include <google/protobuf/message_lite.h>
#include "../../include/message/mlp.pb.h"

void serialize_mlp_model(const MlpModel& mlp_model, std::string& output_str) {
  com::nus::dbsytem::falcon::v0::MlpModel pb_mlp_model;
  pb_mlp_model.set_is_classification(mlp_model.m_is_classification);
  pb_mlp_model.set_num_inputs(mlp_model.m_num_inputs);
  pb_mlp_model.set_num_outputs(mlp_model.m_num_outputs);
  pb_mlp_model.set_num_hidden_layers(mlp_model.m_num_hidden_layers);
  int num_layers_neuron_size = (int) mlp_model.m_layers_num_outputs.size();
  for (int i = 0; i < num_layers_neuron_size; i++) {
    pb_mlp_model.add_num_layers_neurons(mlp_model.m_layers_num_outputs[i]);
  }
  int layer_size = (int) mlp_model.m_layers.size();
  for (int i = 0; i < layer_size; i++) {
    com::nus::dbsytem::falcon::v0::Layer *layer = pb_mlp_model.add_layers();
    layer->set_num_inputs(mlp_model.m_layers[i].m_num_inputs);
    layer->set_num_outputs(mlp_model.m_layers[i].m_num_outputs);
    layer->set_fit_bias(mlp_model.m_layers[i].m_fit_bias);
    layer->set_activation_func_str(mlp_model.m_layers[i].m_activation_func_str);
    auto* enc_matrix = new com::nus::dbsytem::falcon::v0::EncodedNumberMatrix;
    // com::nus::dbsytem::falcon::v0::EncodedNumberMatrix *enc_matrix = layer->mutable_weight_mat();
    for (int j = 0; j < mlp_model.m_layers[i].m_num_inputs; j++) {
      com::nus::dbsytem::falcon::v0::EncodedNumberArray *enc_vec = enc_matrix->add_encoded_array();
      for (int k = 0; k < mlp_model.m_layers[i].m_num_outputs; k++) {
        com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *number = enc_vec->add_encoded_number();
        mpz_t gw_n, gw_value;
        mpz_init(gw_n);
        mpz_init(gw_value);
        mlp_model.m_layers[i].m_weight_mat[j][k].getter_n(gw_n);
        mlp_model.m_layers[i].m_weight_mat[j][k].getter_value(gw_value);
        char *w_n_str_c, *w_value_str_c;
        w_n_str_c = mpz_get_str(NULL, PHE_STR_BASE, gw_n);
        w_value_str_c = mpz_get_str(NULL, PHE_STR_BASE, gw_value);
        std::string w_n_str(w_n_str_c), w_value_str(w_value_str_c);

        number->set_n(w_n_str);
        number->set_value(w_value_str);
        number->set_exponent(mlp_model.m_layers[i].m_weight_mat[j][k].getter_exponent());
        number->set_type(mlp_model.m_layers[i].m_weight_mat[j][k].getter_type());

        mpz_clear(gw_n);
        mpz_clear(gw_value);
        free(w_n_str_c);
        free(w_value_str_c);
      }
    }
    layer->set_allocated_weight_mat(enc_matrix);

//    com::nus::dbsytem::falcon::v0::EncodedNumberArray *enc_array = layer->mutable_bias_vec();
    auto* enc_array = new com::nus::dbsytem::falcon::v0::EncodedNumberArray;
    for (int k = 0; k < mlp_model.m_layers[i].m_num_outputs; k++) {
      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *number = enc_array->add_encoded_number();
      mpz_t gb_n, gb_value;
      mpz_init(gb_n);
      mpz_init(gb_value);
      mlp_model.m_layers[i].m_bias[k].getter_n(gb_n);
      mlp_model.m_layers[i].m_bias[k].getter_value(gb_value);
      char *b_n_str_c, *b_value_str_c;

      b_n_str_c = mpz_get_str(NULL, PHE_STR_BASE, gb_n);
      b_value_str_c = mpz_get_str(NULL, PHE_STR_BASE, gb_value);
      std::string b_n_str(b_n_str_c), b_value_str(b_value_str_c);

      number->set_n(b_n_str);
      number->set_value(b_value_str);
      number->set_exponent(mlp_model.m_layers[i].m_bias[k].getter_exponent());
      number->set_type(mlp_model.m_layers[i].m_bias[k].getter_type());

      mpz_clear(gb_n);
      mpz_clear(gb_value);
      free(b_n_str_c);
      free(b_value_str_c);
    }
    layer->set_allocated_bias_vec(enc_array);
  }
//  std::cout << "mlp_model.m_layers[0].m_weight_mat[0][0].exponent = " << mlp_model.m_layers[0].m_weight_mat[0][0].getter_exponent() << std::endl;
//  std::cout << "mlp_model.m_layers[0].m_weight_mat[0][0].type = " << mlp_model.m_layers[0].m_weight_mat[0][0].getter_type() << std::endl;
//  std::cout << "pb_mlp_model.layers(0).weight_mat().encoded_array(0).encoded_number(0).exponent = "
//    << pb_mlp_model.layers(0).weight_mat().encoded_array(0).encoded_number(0).exponent() << std::endl;
//  std::cout << "pb_mlp_model.layers(0).weight_mat().encoded_array(0).encoded_number(0).type = "
//    << pb_mlp_model.layers(0).weight_mat().encoded_array(0).encoded_number(0).type() << std::endl;

  pb_mlp_model.SerializeToString(&output_str);
  pb_mlp_model.Clear();
}

void deserialize_mlp_model(MlpModel& mlp_model, const std::string& input_str) {
  com::nus::dbsytem::falcon::v0::MlpModel deserialized_mlp_model;
  google::protobuf::io::CodedInputStream inputStream((unsigned char*)input_str.c_str(), input_str.length());
  inputStream.SetTotalBytesLimit(PROTOBUF_SIZE_LIMIT);
  if (!deserialized_mlp_model.ParseFromString(input_str)) {
    log_error("Deserialize mlp model message failed.");
    exit(EXIT_FAILURE);
  }
  mlp_model.m_is_classification = deserialized_mlp_model.is_classification();
  mlp_model.m_num_inputs = deserialized_mlp_model.num_inputs();
  mlp_model.m_num_outputs = deserialized_mlp_model.num_outputs();
  mlp_model.m_num_hidden_layers = deserialized_mlp_model.num_hidden_layers();
  int m_num_layers_neurons_size = deserialized_mlp_model.num_layers_neurons_size();
  for (int i = 0; i < m_num_layers_neurons_size; i++) {
    mlp_model.m_layers_num_outputs.push_back(deserialized_mlp_model.num_layers_neurons(i));
  }
  int m_layers_size = deserialized_mlp_model.layers_size();
  for (int i = 0; i < m_layers_size; i++) {
    const com::nus::dbsytem::falcon::v0::Layer& layer = deserialized_mlp_model.layers(i);
    Layer m_layer;
    m_layer.m_num_inputs = layer.num_inputs();
    m_layer.m_num_outputs = layer.num_outputs();
    m_layer.m_fit_bias = layer.fit_bias();
    m_layer.m_activation_func_str = layer.activation_func_str();
    const com::nus::dbsytem::falcon::v0::EncodedNumberMatrix& enc_matrix = layer.weight_mat();
    const com::nus::dbsytem::falcon::v0::EncodedNumberArray& enc_array = layer.bias_vec();
    m_layer.m_weight_mat = new EncodedNumber*[m_layer.m_num_inputs];
    for (int j = 0; j < m_layer.m_num_inputs; j++) {
      m_layer.m_weight_mat[j] = new EncodedNumber[m_layer.m_num_outputs];
    }
    for (int j = 0; j < m_layer.m_num_inputs; j++) {
      const com::nus::dbsytem::falcon::v0::EncodedNumberArray& enc_vec = enc_matrix.encoded_array(j);
      for (int k = 0; k < m_layer.m_num_outputs; k++) {
        const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& number = enc_vec.encoded_number(k);

        mpz_t s_n, s_value;
        mpz_init(s_n);
        mpz_init(s_value);
        mpz_set_str(s_n, number.n().c_str(), PHE_STR_BASE);
        mpz_set_str(s_value, number.value().c_str(), PHE_STR_BASE);

        m_layer.m_weight_mat[j][k].setter_n(s_n);
        m_layer.m_weight_mat[j][k].setter_value(s_value);
        m_layer.m_weight_mat[j][k].setter_exponent(number.exponent());
        m_layer.m_weight_mat[j][k].setter_type(static_cast<EncodedNumberType>(number.type()));

        mpz_clear(s_n);
        mpz_clear(s_value);
      }
    }

    m_layer.m_bias = new EncodedNumber[m_layer.m_num_outputs];
    for (int k = 0; k < m_layer.m_num_outputs; k++) {
      const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& number = enc_array.encoded_number(k);
      mpz_t s_n, s_value;
      mpz_init(s_n);
      mpz_init(s_value);
      mpz_set_str(s_n, number.n().c_str(), PHE_STR_BASE);
      mpz_set_str(s_value, number.value().c_str(), PHE_STR_BASE);

      m_layer.m_bias[k].setter_n(s_n);
      m_layer.m_bias[k].setter_value(s_value);
      m_layer.m_bias[k].setter_exponent(number.exponent());
      m_layer.m_bias[k].setter_type(static_cast<EncodedNumberType>(number.type()));

      mpz_clear(s_n);
      mpz_clear(s_value);
    }
    mlp_model.m_layers.push_back(m_layer);
  }
}