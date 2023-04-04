#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Task.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "playground/client.h"
#include "playground/work.h"

using namespace folly;
using namespace std::chrono_literals;
using namespace playground;

DEFINE_int32(coro_test_duration_sec, 5, "");
DEFINE_int32(coro_test_qps, 100, "");
DEFINE_int32(coro_test_sync_threadpool_size, 300, "");

coro::Task<void> foo() { co_await folly::futures::sleep(1s); }

TEST(CoroTest, test0) { foo().semi().get(); }

TEST(CoroTest, sync) {
  count = 0;
  Client c(
      Client::Options{.num_workers = 1, .qps = (size_t)FLAGS_coro_test_qps});
  FanoutWork w;
  folly::CPUThreadPoolExecutor ex(FLAGS_coro_test_sync_threadpool_size);
  auto fut =
      c.Pressure([&] { ex.add([&] { w.sync(&ex); }); }).scheduleOn(&ex).start();
  folly::coro::co_invoke([]() -> folly::coro::Task<void> {
    co_await folly::futures::sleep(1s);
    printf("%d\n", (int)count);
    co_return;
  })
      .scheduleOn(&ex)
      .start();
  std::this_thread::sleep_for(
      std::chrono::seconds(FLAGS_coro_test_duration_sec));
  c.stop = true;
  std::move(fut).get();
  // ex.join();
  Metrics::instance().dump(2);
}

TEST(CoroTest, coro) {
  count = 0;
  Client c(
      Client::Options{.num_workers = 1, .qps = (size_t)FLAGS_coro_test_qps});
  folly::CPUThreadPoolExecutor ex(std::thread::hardware_concurrency());
  FanoutWork w;

  auto fut = c.Pressure([&] { w.coro(&ex).scheduleOn(&ex).start(); })
                 .scheduleOn(&ex)
                 .start();
  folly::coro::co_invoke([]() -> folly::coro::Task<void> {
    co_await folly::futures::sleep(1s);
    printf("%d\n", (int)count);
    co_return;
  })
      .scheduleOn(&ex)
      .start();
  std::this_thread::sleep_for(
      std::chrono::seconds(FLAGS_coro_test_duration_sec));
  c.stop = true;
  std::move(fut).get();
  // ex.join();
  Metrics::instance().dump(2);
}