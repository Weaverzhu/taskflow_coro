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

  Client(Options &&opt = Options()) : o(std::move(opt)) {}

  Options o;

  folly::coro::Task<void> Pressure(std::function<void()> &&cob);

  std::atomic_bool stop{false};
};
} // namespace playground