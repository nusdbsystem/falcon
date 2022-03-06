//
// Created by wuyuncheng on 11/12/20.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <numeric>

using namespace std;

#define smoother 0.001

float mean_squared_error(std::vector<float> a, std::vector<float> b);
std::vector<std::vector<float>> read_file(const std::string& file);
inline void check_vectors(std::vector<float> a, std::vector<float> b);

int main(int argc, char* argv[]) {
  // the argv[1] give the comparison algorithm, mse, auc, etc.
  // the argv[2] give the first path file
  // the argv[3] give the second path file
  if (argc < 4) {
    cout << "Args: file_path" << endl;
    return 0;
  }

  std::string alg = argv[1];
  auto first_file = argv[2];
  auto second_file = argv[3];
  cout << "algorithm name = " << alg << endl;
  cout << "first file = " << first_file << endl;
  cout << "second file = " << second_file << endl;

  std::vector<std::vector<float>> first_file_data = read_file(first_file);
  std::vector<std::vector<float>> second_file_data = read_file(second_file);

  int n_line_first = first_file_data.size();
  int n_attributes_first = first_file_data[0].size();
  int n_line_second = second_file_data.size();
  int n_attributes_second = second_file_data[0].size();
  cout << "number of line of the first file: " << n_line_first << endl
       << "number of attributes of the first file: " << n_attributes_first << endl;
  cout << "number of line of the second file: " << n_line_second << endl
       << "number of attributes of the second file: " << n_attributes_second << endl;

  if (alg == "mse") {
    if ((n_line_first != n_line_second) || (n_attributes_first != n_attributes_second)) {
      cout << "the two files are not match" << endl;
      exit(1);
    }
    float avg_mse = 0.0;
    for (int i = 0; i < n_line_first; i++) {
      float mse_i = mean_squared_error(first_file_data[i], second_file_data[i]);
      avg_mse += mse_i;
    }
    avg_mse = avg_mse / n_line_first;
    cout << "average mse = " << avg_mse << endl;
  }

  return 0;
}

float mean_squared_error(std::vector<float> a, std::vector<float> b) {
  check_vectors(a, b);
  int num = a.size();
  float squared_error = 0.0, mean_squared_error = 0.0;
  for (int i = 0; i < num; i++) {
    squared_error = squared_error + (a[i] - b[i]) * (a[i] - b[i]);
    // cout << "a[" << i << "] = " << a[i] << ", b[" << i << "] = " << b[i] << ", squared_error = " << squared_error << endl;
  }
  mean_squared_error = squared_error / num;
  return mean_squared_error;
}

std::vector<std::vector<float>> read_file(const std::string& file) {
  ifstream ifs(file);
  string line;
  vector< vector<float> > table;
  while (getline(ifs, line)) {
    stringstream ss(line);
    string data;
    vector<float> sample;
    while (getline(ss, data, ',')) {
      sample.push_back(atof(data.c_str()));
    }
    table.push_back(sample);
  }
  return table;
}

inline void check_vectors(std::vector<float> a, std::vector<float> b) {
  if (a.size() != b.size()) {
    cout << "Mean squared error computation wrong: sizes of the two "
                  "vectors not same" << endl;
    exit(1);
  }
}