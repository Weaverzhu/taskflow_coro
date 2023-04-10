#include <folly/executors/GlobalExecutor.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/futures/Future.h>
#include <playground/taskflow/taskflow.hpp>

#include <chrono>
#include <gtest/gtest.h>

using playground::taskflow_coro::R;
using namespace std::chrono_literals;

TEST(TaskflowTest, test0) {
  playground::taskflow_coro::Taskflow tf;

  auto &a = tf.emplace([] { puts("a"); });
  for (int i = 0; i < 100; ++i) {
    auto &v = tf.emplace_coro([]() -> R { co_await folly::coro::sleep(1s); });
    a.precede(v);
  }

  tf.run().scheduleOn(folly::getGlobalCPUExecutor()).start().get();
}