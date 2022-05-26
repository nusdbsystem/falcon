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
  pb_mlp_model.set_num_inputs(mlp_model.m_num_inputs);
  pb_mlp_model.set_num_outputs(mlp_model.m_num_outputs);
  pb_mlp_model.set_num_hidden_layers(mlp_model.m_num_hidden_layers);
  int num_layers_neuron_size = (int) mlp_model.m_num_layers_neurons.size();
  for (int i = 0; i < num_layers_neuron_size; i++) {
    pb_mlp_model.add_num_layers_neurons(mlp_model.m_num_layers_neurons[i]);
  }
  int layer_size = (int) mlp_model.m_layers.size();
  for (int i = 0; i < layer_size; i++) {
    com::nus::dbsytem::falcon::v0::Layer *layer = pb_mlp_model.add_layers();
    layer->set_num_neurons(mlp_model.m_layers[i].m_num_neurons);
    layer->set_num_inputs_per_neuron(mlp_model.m_layers[i].m_num_inputs_per_neuron);
    layer->set_activation_func_str(mlp_model.m_layers[i].m_activation_func_str);
    int neuron_size = (int) mlp_model.m_layers[i].m_neurons.size();
    for (int j = 0; j < neuron_size; j++) {
      com::nus::dbsytem::falcon::v0::Neuron *neuron = layer->add_neurons();
      neuron->set_num_inputs(mlp_model.m_layers[i].m_neurons[j].m_num_inputs);
      neuron->set_fit_bias(mlp_model.m_layers[i].m_neurons[j].m_fit_bias);
      // add one bias term
      com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *encoded_number = neuron->add_bias();
      mpz_t g_n, g_value;
      mpz_init(g_n);
      mpz_init(g_value);
      mlp_model.m_layers[i].m_neurons[j].m_bias[0].getter_n(g_n);
      mlp_model.m_layers[i].m_neurons[j].m_bias[0].getter_value(g_value);
      char *n_str_c, *value_str_c;
      n_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_n);
      value_str_c = mpz_get_str(NULL, PHE_STR_BASE, g_value);
      std::string n_str(n_str_c), value_str(value_str_c);

      encoded_number->set_n(n_str);
      encoded_number->set_value(value_str);
      encoded_number->set_exponent(mlp_model.m_layers[i].m_neurons[j].m_bias[0].getter_exponent());
      encoded_number->set_type(mlp_model.m_layers[i].m_neurons[j].m_bias[0].getter_type());

      mpz_clear(g_n);
      mpz_clear(g_value);
      free(n_str_c);
      free(value_str_c);

      // add the weights vector
      int neuron_input_size = mlp_model.m_layers[i].m_neurons[j].m_num_inputs;
      for (int k = 0; k < neuron_input_size; k++) {
        com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber *number = neuron->add_weights();
        mpz_t gw_n, gw_value;
        mpz_init(gw_n);
        mpz_init(gw_value);
        mlp_model.m_layers[i].m_neurons[j].m_bias[0].getter_n(g_n);
        mlp_model.m_layers[i].m_neurons[j].m_bias[0].getter_value(g_value);
        char *w_n_str_c, *w_value_str_c;
        w_n_str_c = mpz_get_str(NULL, PHE_STR_BASE, gw_n);
        w_value_str_c = mpz_get_str(NULL, PHE_STR_BASE, gw_value);
        std::string w_n_str(w_n_str_c), w_value_str(w_value_str_c);

        number->set_n(w_n_str);
        number->set_value(w_value_str);
        number->set_exponent(mlp_model.m_layers[i].m_neurons[j].m_weights[k].getter_exponent());
        number->set_type(mlp_model.m_layers[i].m_neurons[j].m_weights[k].getter_type());

        mpz_clear(gw_n);
        mpz_clear(gw_value);
        free(w_n_str_c);
        free(w_value_str_c);
      }
    }
  }
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
  mlp_model.m_num_inputs = deserialized_mlp_model.num_inputs();
  mlp_model.m_num_outputs = deserialized_mlp_model.num_outputs();
  mlp_model.m_num_hidden_layers = deserialized_mlp_model.num_hidden_layers();
  int m_num_layers_neurons_size = deserialized_mlp_model.num_layers_neurons_size();
  for (int i = 0; i < m_num_layers_neurons_size; i++) {
    mlp_model.m_num_layers_neurons.push_back(deserialized_mlp_model.num_layers_neurons(i));
  }
  int m_layers_size = deserialized_mlp_model.layers_size();
  for (int i = 0; i < m_layers_size; i++) {
    const com::nus::dbsytem::falcon::v0::Layer& layer = deserialized_mlp_model.layers(i);
    Layer m_layer;
    m_layer.m_num_neurons = layer.num_neurons();
    m_layer.m_num_inputs_per_neuron = layer.num_inputs_per_neuron();
    m_layer.m_activation_func_str = layer.activation_func_str();
    int m_neuron_size = layer.neurons_size();
    for (int j = 0; j < m_neuron_size; j++) {
      const com::nus::dbsytem::falcon::v0::Neuron& neuron = layer.neurons(j);
      Neuron m_neuron;
      m_neuron.m_num_inputs = neuron.num_inputs();
      m_neuron.m_fit_bias = neuron.fit_bias();
      // copy bias term
      m_neuron.m_bias = new EncodedNumber[1];
      const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& bias0 = neuron.bias(0);
      mpz_t s_n_bias, s_value_bias;
      mpz_init(s_n_bias);
      mpz_init(s_value_bias);
      mpz_set_str(s_n_bias, bias0.n().c_str(), PHE_STR_BASE);
      mpz_set_str(s_value_bias, bias0.value().c_str(), PHE_STR_BASE);

      m_neuron.m_bias[0].setter_n(s_n_bias);
      m_neuron.m_bias[0].setter_value(s_value_bias);
      m_neuron.m_bias[0].setter_exponent(bias0.exponent());
      m_neuron.m_bias[0].setter_type(static_cast<EncodedNumberType>(bias0.type()));

      mpz_clear(s_n_bias);
      mpz_clear(s_value_bias);

      // copy weights
      int m_weights_size = neuron.weights_size();
      m_neuron.m_weights = new EncodedNumber[m_weights_size];
      for (int k = 0; k < m_weights_size; k++) {
        const com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& encoded_number = neuron.weights(i);
        mpz_t s_n, s_value;
        mpz_init(s_n);
        mpz_init(s_value);
        mpz_set_str(s_n, encoded_number.n().c_str(), PHE_STR_BASE);
        mpz_set_str(s_value, encoded_number.value().c_str(), PHE_STR_BASE);

        m_neuron.m_weights[i].setter_n(s_n);
        m_neuron.m_weights[i].setter_value(s_value);
        m_neuron.m_weights[i].setter_exponent(encoded_number.exponent());
        m_neuron.m_weights[i].setter_type(static_cast<EncodedNumberType>(encoded_number.type()));

        mpz_clear(s_n);
        mpz_clear(s_value);
      }
      m_layer.m_neurons.push_back(m_neuron);
    }
    mlp_model.m_layers.push_back(m_layer);
  }
}