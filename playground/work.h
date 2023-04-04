#pragma once

#include "playground/stats.h"

#include <folly/experimental/coro/Task.h>

namespace playground {

extern std::atomic_int64_t count;

struct Work {
  virtual void sync(folly::Executor *ex);

  virtual folly::coro::Task<void> coro(folly::Executor *ex);
};

struct FanoutWork : public Work {
  void sync(folly::Executor *ex) override;

  folly::coro::Task<void> coro(folly::Executor *ex) override;
};

} // namespace playground