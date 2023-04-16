#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/executors/InlineExecutor.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Task.h>
#include <folly/futures/Future.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>

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

struct DebugExecutor : public folly::CPUThreadPoolExecutor {
  std::atomic_size_t addCnt{0};
  bool printEveryAdd = false;

  DebugExecutor(size_t num) : folly::CPUThreadPoolExecutor(num) {}

  virtual void add(folly::Func func) override {
    addCnt += 1;
    if (printEveryAdd) {
      printf("[DebugExecutor %p] add\n", this);
    }
    folly::CPUThreadPoolExecutor::add(std::move(func));
  }
};

namespace future {
folly::Future<int> baz() {
  folly::Promise<int> p;
  auto f = p.getFuture();
  std::thread t([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(1s);
    p.setValue(456);
  });
  t.detach();
  return f;
}
folly::Future<int> bar() { return 123; }
folly::Future<int> foo() {
  return bar().thenValue([](int &&ret) { return baz(); });
}
} // namespace future

TEST(CoroAwaitTest, test0) {
  DebugExecutor ex(32);
  auto futureFunc = []() -> folly::Future<int> { return 123; };

  folly::coro::co_invoke([&]() -> folly::coro::Task<int> {
    co_await futureFunc();
    co_return 0;
  })
      .scheduleOn(&ex)
      .start()
      .get();
  std::cout << ex.addCnt << std::endl;
}

TEST(CoroAwaitTest, coro) {
  DebugExecutor ex(32);
  folly::coro::co_invoke([]() -> folly::coro::Task<void> {
    co_await future::foo();
    co_return;
  })
      .scheduleOn(&ex)
      .start()
      .get();
  std::cout << ex.addCnt << std::endl;
}

TEST(CoroAddTest, coro) {
  DebugExecutor ex(std::thread::hardware_concurrency());

  std::function<folly::coro::Task<int>(int)> fib_coro =
      [&](int n) -> folly::coro::Task<int> {
    if (n <= 2) {
      co_return n;
    }
    auto [res1, res2] =
        co_await folly::coro::collectAll(fib_coro(n - 1).scheduleOn(&ex),
                                         fib_coro(n - 2).scheduleOn(&ex))
            .scheduleOn(&folly::InlineExecutor::instance());

    co_return res1 + res2;
  };

  fib_coro(3).scheduleOn(&ex).start().get();
  std::cout << ex.addCnt << std::endl;
}

TEST(CoroAddTest, future) {
  DebugExecutor ex(std::thread::hardware_concurrency());
  std::function<folly::SemiFuture<int>(int)> fib_future =
      [&](int n) -> folly::Future<int> {
    if (n <= 2) {
      return n;
    }
    std::array<folly::SemiFuture<int>, 2> futs = {fib_future(n - 1),
                                                  fib_future(n - 2).via(&ex)};
    return folly::collectAll(futs.begin(), futs.end())
        .via(&folly::InlineExecutor::instance())
        .thenValue([](std::vector<folly::Try<int>> &&ret) {
          return ret[0].value() + ret[1].value();
        });
  };

  folly::via(&ex)
      .thenValue([&](folly::Unit &&) { return fib_future(3); })
      .get();
  std::cout << ex.addCnt << std::endl;
}