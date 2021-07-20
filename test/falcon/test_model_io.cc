//
// Created by wuyuncheng on 3/2/21.
//

#include <vector>

#include <falcon/model/model_io.h>

#include <gtest/gtest.h>

TEST(Model_IO, SaveModelString) {
  std::string model_string = "This is an example of the model";
  std::string save_model_file = "saved_model.txt";
  save_pb_model_string(model_string, save_model_file);
  std::string loaded_model_string;
  load_pb_model_string(loaded_model_string, save_model_file);
  EXPECT_TRUE(model_string == loaded_model_string);
}