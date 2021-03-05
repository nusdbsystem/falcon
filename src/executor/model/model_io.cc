//
// Created by wuyuncheng on 2/2/21.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <falcon/model/model_io.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/base64.h>

void save_lr_model(EncodedNumber* model_weights, int weight_size, const std::string& model_save_path){
  std::ofstream write_outfile(model_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  std::string local_model_str;

  // print saved model weights
  //  for (int i = 0; i < local_model_size; i++) {
  //    mpz_t deserialized_n, deserialized_value;
  //    mpz_init(deserialized_n);
  //    mpz_init(deserialized_value);
  //    local_model_weights[i].getter_n(deserialized_n);
  //    local_model_weights[i].getter_value(deserialized_value);
  //    gmp_printf("model weights %d n = %Zd\n", i, deserialized_n);
  //    gmp_printf("model weights %d value = %Zd\n", i, deserialized_value);
  //    mpz_clear(deserialized_n);
  //    mpz_clear(deserialized_value);
  //  }

  serialize_encoded_number_array(model_weights, weight_size, local_model_str);
  std::string b64_local_model_str = base64_encode(reinterpret_cast<const BYTE *>(local_model_str.c_str()), local_model_str.size());

  write_outfile << weight_size << "\n";
  write_outfile << b64_local_model_str << "\n";
  write_outfile.close();
}

void load_lr_model(const std::string& model_save_path, int& weight_size, EncodedNumber* saved_weights) {
  std::ifstream model_infile(model_save_path);
  if (!model_infile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string line;
  int line_num = 0;
  while (std::getline(model_infile, line)) {
    if (line_num == 0) {
      // read weight size
      weight_size = ::atoi(line.c_str());
    } else if (line_num == 1) {
      // read weights string
      vector<BYTE> saved_model_vec = base64_decode(line.c_str());
      std::string s(saved_model_vec.begin(), saved_model_vec.end());
      deserialize_encoded_number_array(saved_weights, weight_size, s);
    } else {
      break;
    }
    line_num ++;
  }

  // print saved model weights
  //  for (int i = 0; i < weight_size; i++) {
  //    mpz_t deserialized_n, deserialized_value;
  //    mpz_init(deserialized_n);
  //    mpz_init(deserialized_value);
  //    saved_weights[i].getter_n(deserialized_n);
  //    saved_weights[i].getter_value(deserialized_value);
  //    gmp_printf("model weights %d n = %Zd\n", i, deserialized_n);
  //    gmp_printf("model weights %d value = %Zd\n", i, deserialized_value);
  //    mpz_clear(deserialized_n);
  //    mpz_clear(deserialized_value);
  //  }

  model_infile.close();
}

void save_lr_report(LogisticRegression lr_model, const std::string& report_save_path) {
  std::ofstream write_outfile(report_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << report_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  float training_accuracy = lr_model.getter_training_accuracy();
  float testing_accuracy = lr_model.getter_testing_accuracy();
  write_outfile << "The training accuracy of this model is: " << training_accuracy << "\n";
  write_outfile << "The testing accuracy of this model is: " << testing_accuracy << "\n";
  write_outfile.close();
}