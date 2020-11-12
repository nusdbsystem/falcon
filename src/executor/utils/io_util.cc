//
// Created by wuyuncheng on 12/11/20.
//

#include "falcon/utils/io_util.h"

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
  return data;
}