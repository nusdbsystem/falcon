//
// Created by wuyuncheng on 11/12/20.
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define smoother 0.001

int main(int argc, char *argv[]) {
  // the argv[1] give the file path
  // the argv[2] denote whether normalize labels, for classification, no,
  // regression, yes
  if (argc < 3) {
    cout << "Args: file_path" << endl;
    return 0;
  }

  auto file = argv[1];
  auto flag = atoi(argv[2]);
  cout << "flag = " << flag << endl;
  ifstream ifs(file);
  string line;
  vector<vector<float>> table;
  while (getline(ifs, line)) {
    stringstream ss(line);
    string data;
    vector<float> sample;
    while (getline(ss, data, ',')) {
      sample.push_back(atof(data.c_str()));
    }
    table.push_back(sample);
  }
  int n_samples = table.size();
  int n_attributes = table[0].size();
  cout << "number of samples: " << n_samples << endl
       << "number of attributes (including label): " << n_attributes << endl;

  std::vector<float> attributes_max;
  std::vector<float> attributes_min;
  for (int i = 0; i < n_attributes - 1; i++) {
    attributes_max.push_back(INT32_MIN);
    attributes_min.push_back(INT32_MAX);
  }

  int normalized_attribute = 0;
  if (flag == 0) {
    normalized_attribute = n_attributes - 1;
  } else {
    normalized_attribute = n_attributes;
  }
  cout << "normalized attribute number: " << normalized_attribute << endl;
  for (int i = 0; i < n_samples; i++) {
    for (int j = 0; j < normalized_attribute; j++) {
      if (table[i][j] > attributes_max[j]) {
        attributes_max[j] = table[i][j];
      }
      if (table[i][j] < attributes_min[j]) {
        attributes_min[j] = table[i][j];
      }
    }
  }

  for (int i = 0; i < n_samples; i++) {
    for (int j = 0; j < normalized_attribute; j++) {
      table[i][j] = (table[i][j] - attributes_min[j] + smoother) /
                    (attributes_max[j] - attributes_min[j] + smoother);
    }
  }

  // write to the file
  std::string outfile = string(file) + ".norm";
  std::cout << "outfile name: " << outfile << endl;
  ofstream out1;
  out1.open(outfile);
  out1.precision(6);
  for (auto entry : table) {
    for (int j = 0; j < n_attributes; j++) {
      out1 << fixed << entry[j];
      if (j < n_attributes - 1) {
        out1 << ",";
      } else {
        out1 << endl;
      }
    }
  }
  out1.flush();
  cout << "write finished " << endl;
  return 0;
}