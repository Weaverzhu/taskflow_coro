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

coro::Task<void> foo() {
  puts("before sleep");
  co_await folly::futures::sleep(1s);
  printf("after sleep ");

  std::cout << std::this_thread::get_id() << std::endl;
  co_return;
}
coro::Task<void> bar() {
  puts("before foo * 2");
  co_await folly::coro::collectAll(foo().semi(), foo().semi())
      .scheduleOn(&folly::InlineExecutor::instance());
  puts("after foo * 2");

  std::cout << std::this_thread::get_id() << std::endl;
  co_return;
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

TEST(CoroTest, test0) { foo().semi().get(); }

TEST(CoroTest, test1) {
  DebugExecutor ex(1);
  ex.printEveryAdd = true;
  folly::coro::co_invoke([&]() -> folly::coro::Task<void> {
    std::cout << std::this_thread::get_id() << std::endl;
    puts("before bar");
    co_await bar();
    puts("after bar");
    co_return;
  })
      .scheduleOn(&ex)
      .start()
      .get();
  printf("%lu\n", ex.addCnt.load());
}

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
    auto [res1, res2] = co_await folly::coro::collectAll(
        fib_coro(n - 1).scheduleOn(&ex), fib_coro(n - 2).scheduleOn(&ex));

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
