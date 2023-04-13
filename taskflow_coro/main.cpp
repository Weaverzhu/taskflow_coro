#include "folly/executors/GlobalExecutor.h"
#include <folly/experimental/coro/Sleep.h>
#include <folly/init/Init.h>
#include <gtest/gtest.h>

#include <taskflow_coro/taskflow.hpp>

using R = folly::coro::Task<void>;

using namespace std::chrono_literals;

/*
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from TaskflowTest
[ RUN      ] TaskflowTest.coro
[       OK ] TaskflowTest.coro (3008 ms)
[----------] 1 test from TaskflowTest (3008 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (3008 ms total)
[  PASSED  ] 1 test.
*/

TEST(TaskflowTest, coro) {
  taskflow_coro::Taskflow tf;
  auto &source = tf.emplace_coro([]() -> R {
    co_await folly::coro::sleep(1s);
    co_return;
  });
  auto &dest = tf.emplace_coro([]() -> R {
    co_await folly::coro::sleep(1s);
    co_return;
  });
  for (int i = 0; i < 1000; ++i) {
    auto &internalIOTask = tf.emplace_coro([]() -> R {
      co_await folly::coro::sleep(1s);
      co_return;
    });
    source.precede(internalIOTask);
    internalIOTask.precede(dest);
  }

  tf.run().scheduleOn(folly::getGlobalCPUExecutor()).start().get();
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  folly::init(&argc, &argv);

  return RUN_ALL_TESTS();
}