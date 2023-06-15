/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 2/2/21.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <falcon/model/model_io.h>
#include <falcon/utils/base64.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>

void save_pb_model_string(const std::string &pb_model_string,
                          const std::string &model_save_path) {
  std::ofstream write_outfile(model_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << model_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  std::string b64_local_model_str =
      base64_encode(reinterpret_cast<const BYTE *>(pb_model_string.c_str()),
                    pb_model_string.size());
  write_outfile << b64_local_model_str << "\n";
  write_outfile.close();
}

void load_pb_model_string(std::string &saved_pb_model_string,
                          const std::string &model_save_path) {
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
    saved_pb_model_string = s;
  }
  model_infile.close();
}

void save_training_report(double training_accuracy, double testing_accuracy,
                          const std::string &report_save_path) {
  std::ofstream write_outfile(report_save_path);
  if (!write_outfile) {
    LOG(INFO) << "Open " << report_save_path.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  write_outfile << "The training accuracy of this model is: "
                << training_accuracy << "\n";
  write_outfile << "The testing accuracy of this model is: " << testing_accuracy
                << "\n";
  write_outfile.close();
}