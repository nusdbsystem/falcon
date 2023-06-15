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
#define MAX_CLIENT 10

int main(int argc, char *argv[]) {
  if (argc < 3) {
    cout << "Args: file_path" << endl;
    return 0;
  }

  auto file = argv[1];
  int num_client = atoi(argv[2]);
  ifstream ifs(file);
  string line;
  vector<vector<string>> table;
  while (getline(ifs, line)) {
    stringstream ss(line);
    string data;
    vector<string> sample;
    while (getline(ss, data, ',')) {
      // if (data == "0") data = "0.00001"; // temporarily fix zero value issue
      sample.push_back(data);
    }
    table.push_back(sample);
  }
  int n_samples = table.size();
  int n_attributes = table[0].size();
  cout << "number of samples: " << n_samples << endl
       << "number of attributes (without label): " << n_attributes << endl;

  int n_attr_per_client = n_attributes / num_client;
  cout << "n_attr_per_client = " << n_attr_per_client << endl;

  std::vector<std::string> files;
  ofstream outs[MAX_CLIENT];
  for (int i = 0; i < num_client; i++) {
    std::string f = "client_" + to_string(i) + ".txt";
    files.push_back(f);
    outs[i].open(f);
    outs[i].precision(6);
  }

  int threshold_index[MAX_CLIENT];
  for (int i = 0; i < num_client; i++) {
    int threshold;
    if (i < num_client - 1) {
      threshold = n_attr_per_client * (i + 1);
    } else {
      threshold = n_attributes;
    }
    threshold_index[i] = threshold;
    cout << "threshold index " << i << " = " << threshold_index[i] << endl;
  }
  threshold_index[num_client] = 0;
  // threshold_index[0] = threshold_index[0] + 1; // include label

  for (auto entry : table) {
    for (int i = 0; i < n_attributes; ++i) {
      for (int client_id = 0; client_id < num_client; client_id++) {
        if (i < threshold_index[client_id] &&
            i >= threshold_index[client_id - 1]) {
          double d = std::stod(entry[i]);
          outs[client_id] << fixed << d;
          if (i == threshold_index[client_id] - 1) {
            outs[client_id] << endl;
          } else {
            outs[client_id] << ",";
          }
        }
      }
    }
  }

  return 0;
}
