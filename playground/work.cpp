#include "playground/work.h"

#include <folly/experimental/coro/Collect.h>

#include <iostream>

namespace playground {

void init(int n, std::vector<std::vector<double>> &a) {
  a.resize(n);
  for (auto &x : a) {
    x.resize(n);
    for (double &y : x) {
      y = rand();
    }
  }
}

void gemm(int n) {
  std::vector<std::vector<double>> a, b;

  init(n, a);
  init(n, b);

  std::vector<std::vector<double>> c(n, std::vector<double>(n, 0));

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      c[i][j] = 0;
      for (int k = 0; k < n; ++k) {
        c[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

void Work::sync(folly::Executor *ex) {
  if (rand() % 2) {
    gemm(20);
  } else {
    folly::futures::sleep(std::chrono::milliseconds(10 + rand() % 10)).get();
  }
}

folly::coro::Task<void> Work::coro(folly::Executor *ex) {
  if (rand() % 2) {
    gemm(20);
  } else {
    co_await folly::futures::sleep(std::chrono::milliseconds(10 + rand() % 10));
  }
}

static int n = 100;

std::atomic_int64_t count{0};

void FanoutWork::sync(folly::Executor *ex) {
  auto start = std::chrono::system_clock::now();
  std::vector<folly::Future<int>> futs;
  futs.reserve(n);
  for (int i = 0; i < n; ++i) {
    folly::Promise<int> p;
    futs.emplace_back(p.getFuture());
    ex->add([p = std::move(p), ex]() mutable {
      Work w;
      w.sync(ex);
      p.setValue(0);
    });
  }

  for (int i = 0; i < n; ++i) {
    std::move(futs[i]).get();
  }
  Metrics::instance().summary(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now() - start));
  ++count;
}

folly::coro::Task<void> FanoutWork::coro(folly::Executor *ex) {
  auto start = std::chrono::system_clock::now();
  std::vector<folly::coro::TaskWithExecutor<int>> coros;
  coros.reserve(n);
  for (int i = 0; i < n; ++i) {
    coros.emplace_back(folly::coro::co_invoke([ex]() -> folly::coro::Task<int> {
                         Work w;
                         co_await w.coro(ex);
                         co_return 0;
                       }).scheduleOn(ex));
  }
  co_await folly::coro::collectAllRange(std::move(coros));

  Metrics::instance().summary(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now() - start));
  ++count;
  co_return;
}

} // namespace playground