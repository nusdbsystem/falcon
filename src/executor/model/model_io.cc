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

void save_lr_model(LogisticRegression lr_model, const std::string& model_save_path) {
  std::ofstream write_outfile(model_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  // retrieve model weights and save into file
  int local_model_size = lr_model.getter_weight_size();
  std::string local_model_str;
  EncodedNumber * local_model_weights = new EncodedNumber[local_model_size];
  lr_model.getter_encoded_weights(local_model_weights);
  serialize_encoded_number_array(local_model_weights, local_model_size, local_model_str);

  write_outfile << local_model_size << "\n";
  write_outfile << local_model_str << "\n";
  write_outfile.close();
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