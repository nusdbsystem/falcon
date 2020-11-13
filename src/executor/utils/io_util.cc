//
// Created by wuyuncheng on 12/11/20.
//

#include <falcon/utils/io_util.h>

#include <iterator>

std::vector< std::vector<float> > read_dataset(const std::string& data_file) {
  std::ifstream data_infile(data_file);
  if (!data_infile) {
    LOG(INFO) << "Open " << data_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::vector< std::vector<float> > data;
  std::string line;
  while (std::getline(data_infile, line)) {
    std::vector<float> items;
    std::istringstream ss(line);
    std::string item;
    // split line with delimiter, default ','
    while(getline(ss, item,','))
    {
      items.push_back(::atof(item.c_str()));
    }
    data.push_back(items);
  }
  data_infile.close();

  return data;
}

std::string read_key_file(const std::string& key_file) {
  std::ifstream key_infile(key_file);
  if (!key_infile) {
    LOG(INFO) << "Open " << key_file.c_str() << " file error.";
    exit(EXIT_FAILURE);
  }

  std::string phe_keys_str;
  copy(std::istreambuf_iterator<char>(key_infile),
       std::istreambuf_iterator<char>(),
       back_inserter(phe_keys_str));
  key_infile.close();

  return phe_keys_str;
}