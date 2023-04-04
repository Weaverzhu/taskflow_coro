#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Task.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "playground/client.h"
#include "playground/work.h"

using namespace folly;
using namespace std::chrono_literals;
using namespace playground;

DEFINE_int32(coro_test_duration_sec, 30, "");
DEFINE_int32(coro_test_qps, 100, "");

coro::Task<void> foo() { co_await folly::futures::sleep(1s); }

TEST(CoroTest, test0) { foo().semi().get(); }

TEST(CoroTest, sync) {
  Client c(
      Client::Options{.num_workers = 1, .qps = (size_t)FLAGS_coro_test_qps});
  FanoutWork w;
  folly::coro::co_invoke([]() -> folly::coro::Task<void> {
    co_await folly::futures::sleep(1s);
    printf("%d\n", (int)count);
    co_return;
  })
      .scheduleOn(folly::getGlobalCPUExecutor())
      .start();
  auto fut =
      c.Pressure([&] { folly::getGlobalCPUExecutor()->add([&] { w.sync(); }); })
          .scheduleOn(folly::getGlobalCPUExecutor())
          .start();
  std::this_thread::sleep_for(
      std::chrono::seconds(FLAGS_coro_test_duration_sec));
  c.stop = true;
  std::move(fut).get();
  Metrics::instance().dump(2);
}

TEST(CoroTest, coro) {
  Client c(
      Client::Options{.num_workers = 1, .qps = (size_t)FLAGS_coro_test_qps});
  FanoutWork w;
  folly::coro::co_invoke([]() -> folly::coro::Task<void> {
    co_await folly::futures::sleep(1s);
    printf("%d\n", (int)count);
    co_return;
  })
      .scheduleOn(folly::getGlobalCPUExecutor())
      .start();
  auto fut = c.Pressure([&] {
                w.coro().scheduleOn(folly::getGlobalCPUExecutor()).start();
              })
                 .scheduleOn(folly::getGlobalCPUExecutor())
                 .start();
  std::this_thread::sleep_for(
      std::chrono::seconds(FLAGS_coro_test_duration_sec));
  c.stop = true;
  std::move(fut).get();
  Metrics::instance().dump(2);
}