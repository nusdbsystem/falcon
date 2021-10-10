//
// Created by wuyuncheng on 12/11/20.
//

#include <falcon/utils/io_util.h>
#include <iterator>

std::vector<std::vector<double> > read_dataset(const std::string& data_file,
                                               char delimiter) {
  std::ifstream data_infile(data_file);
  if (!data_infile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::vector<std::vector<double> > data;
  std::string line;
  while (std::getline(data_infile, line)) {
    std::vector<double> items;
    std::istringstream ss(line);
    std::string item;
    // split line with delimiter, default ','
    while (getline(ss, item, delimiter)) {
      items.push_back(::atof(item.c_str()));
    }
    data.push_back(items);
  }
  data_infile.close();

  return data;
}

void write_dataset_to_file(std::vector<std::vector<double> > data,
                           char delimiter, const std::string& data_file) {
  std::ofstream write_outfile(data_file);
  if (!write_outfile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }
  int row_num = data.size();
  int column_num = data[0].size();
  for (int i = 0; i < row_num; i++) {
    std::string line;
    for (int j = 0; j < column_num; j++) {
      // std::to_string defaults to 6 decimal places
      // when given an input value of type float or double
      line = line + std::to_string(data[i][j]);
      if (j != column_num - 1) {
        line += delimiter;
      } else {
        line += "\n";
      }
    }
    write_outfile << line;
  }
  write_outfile.close();
}

// for Party::split_train_test_data
// save a copy of shuffled data_indexes vector<int> to file
// for local debugging
void write_shuffled_data_indexes_to_file(std::vector<int> data_indexes,
                                         const std::string& data_file) {
  std::ofstream write_outfile(data_file);
  if (!write_outfile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  // ostream_iterator for stream cout
  // with new line delimiter
  std::ostream_iterator<int> output_iterator(write_outfile, "\n");
  std::copy(data_indexes.begin(), data_indexes.end(), output_iterator);

  write_outfile.close();
}

std::string read_key_file(const std::string& key_file) {
  std::ifstream key_infile(key_file);
  if (!key_infile) {
    LOG(INFO) << "Open " << key_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string phe_keys_str;
  copy(std::istreambuf_iterator<char>(key_infile),
       std::istreambuf_iterator<char>(), back_inserter(phe_keys_str));
  key_infile.close();

  return phe_keys_str;
}

void write_key_to_file(std::string phe_keys_str, const std::string& key_file) {
  std::ofstream write_outfile(key_file);
  if (!write_outfile) {
    LOG(INFO) << "Open " << key_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  write_outfile << phe_keys_str;
  write_outfile.close();
}
