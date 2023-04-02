#include <folly/init/Init.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  folly::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
