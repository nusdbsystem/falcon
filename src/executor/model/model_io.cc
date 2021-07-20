//
// Created by wuyuncheng on 2/2/21.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <falcon/model/model_io.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/base64.h>

void save_lr_model(LogisticRegressionModel lr_model, const std::string& model_save_path){
  std::ofstream write_outfile(model_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  std::string local_model_str;

  serialize_lr_model(lr_model, local_model_str);
  std::string b64_local_model_str = base64_encode(reinterpret_cast<const BYTE *>(local_model_str.c_str()), local_model_str.size());

  write_outfile << b64_local_model_str << "\n";
  write_outfile.close();
}

void load_lr_model(const std::string& model_save_path, LogisticRegressionModel& saved_lr_model) {
  std::ifstream model_infile(model_save_path);
  if (!model_infile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string line;
  while (std::getline(model_infile, line)) {
    // read weights string
    vector<BYTE> saved_model_vec = base64_decode(line.c_str());
    std::string s(saved_model_vec.begin(), saved_model_vec.end());
    deserialize_lr_model(saved_lr_model, s);
  }

  model_infile.close();
}

void save_dt_model(TreeModel tree, const std::string& model_save_path) {
  std::ofstream write_outfile(model_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  std::string local_model_str;
  serialize_tree_model(tree, local_model_str);
  std::string b64_local_model_str = base64_encode(reinterpret_cast<const BYTE *>(
      local_model_str.c_str()), local_model_str.size());
  write_outfile << b64_local_model_str << "\n";
  write_outfile.close();
}

void load_dt_model(const std::string& model_save_path, TreeModel& saved_tree) {
  std::ifstream model_infile(model_save_path);
  if (!model_infile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string line;
  while (std::getline(model_infile, line)) {
    // read weights string
    vector<BYTE> saved_model_vec = base64_decode(line.c_str());
    std::string s(saved_model_vec.begin(), saved_model_vec.end());
    deserialize_tree_model(saved_tree, s);
  }

  model_infile.close();
}

void save_rf_model(ForestModel forest_model, const std::string& model_save_path) {
  std::ofstream write_outfile(model_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  std::string local_model_str;
  serialize_random_forest_model(forest_model, local_model_str);
  std::string b64_local_model_str = base64_encode(reinterpret_cast<const BYTE *>(local_model_str.c_str()), local_model_str.size());
  write_outfile << b64_local_model_str << "\n";
  write_outfile.close();
}

void load_rf_model(const std::string& model_save_path, ForestModel& saved_forest_model) {
  std::ifstream model_infile(model_save_path);
  if (!model_infile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string line;
  while (std::getline(model_infile, line)) {
    // read weights string
    vector<BYTE> saved_model_vec = base64_decode(line.c_str());
    std::string s(saved_model_vec.begin(), saved_model_vec.end());
    deserialize_random_forest_model(saved_forest_model, s);
  }

  model_infile.close();
}

void save_training_report(double training_accuracy,
    double testing_accuracy,
    const std::string& report_save_path) {
  std::ofstream write_outfile(report_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << report_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  write_outfile << "The training accuracy of this model is: " << training_accuracy << "\n";
  write_outfile << "The testing accuracy of this model is: " << testing_accuracy << "\n";
  write_outfile.close();
}