#pragma once
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/experimental/coro/Task.h>

namespace playground {

struct Client {
  struct Options {
    size_t num_workers;
    size_t qps;
  };

  Client(Options &&opt = Options())
      : o(std::move(opt)), ex(opt.num_workers), ps(opt.num_workers) {}

  Options o;

  folly::coro::Task<void> Pressure(std::function<void()>);

  std::atomic_bool stop{false};

  folly::CPUThreadPoolExecutor ex;
  std::vector<folly::Promise<folly::Unit>> ps;
};
} // namespace playground