#pragma once

#include "playground/stats.h"

#include <folly/experimental/coro/Task.h>

namespace playground {

extern std::atomic_int64_t count;

struct Work {
  virtual void sync();

  virtual folly::coro::Task<void> coro();
};

struct FanoutWork : public Work {
  void sync() override;

  folly::coro::Task<void> coro() override;
};

} // namespace playground