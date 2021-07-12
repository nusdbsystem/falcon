//
// Created by wuyuncheng on 30/10/20.
//

#include <falcon/common.h>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  // create the IO test folder
  int stat;
  stat = mkdir(TEST_IO_OUTDIR, 0777);
  if (!stat)
    std::cout << "TEST_IO_OUTDIR created\n";
  else {
    std::cout << "Unable to create TEST_IO_OUTDIR\n";
    exit(EXIT_FAILURE);
  }
  return RUN_ALL_TESTS();
}