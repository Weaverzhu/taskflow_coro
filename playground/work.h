#pragma once

#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/experimental/coro/Task.h>

namespace playground {

struct Work {
  Work(folly::Executor::KeepAlive<> ex) : ex(ex) {}

  folly::Executor::KeepAlive<> ex;

  virtual void sync() = 0;

  virtual folly::coro::Task<void> coro() {
    sync();
    co_return;
  }
};

struct IOWork : public Work {
  size_t sleep_ms;
  IOWork(size_t sleep_ms, folly::Executor::KeepAlive<> ex)
      : Work(ex), sleep_ms(sleep_ms) {}

  void sync() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
  }

  folly::coro::Task<void> coro() override {
    co_await folly::coro::sleep(std::chrono::milliseconds(sleep_ms));
  }
};

struct MemCPUWork : public Work {
  size_t n;
  MemCPUWork(size_t n, folly::Executor::KeepAlive<> ex) : Work(ex), n(n){};

  virtual void sync() override {
    std::unordered_set<size_t> s;
    for (size_t i = 0; i < n; ++i) {
      s.insert(rand() % (n / 10));
    }
  }
};

struct FanoutWork : public Work {
  FanoutWork(std::vector<std::shared_ptr<Work>> &&works,
             folly::Executor::KeepAlive<> ex)
      : Work(ex), works(std::move(works)) {}

  std::vector<std::shared_ptr<Work>> works;

  void sync() override {
    std::vector<folly::Promise<folly::Unit>> ps(works.size());
    std::vector<folly::SemiFuture<folly::Unit>> futs;
    futs.reserve(works.size());
    for (size_t i = 0; i < ps.size(); ++i) {
      futs.push_back(ps[i].getSemiFuture());
      ex->add([p = std::move(ps[i]), w = works[i], ex = this->ex]() mutable {
        w->sync();
        p.setValue(folly::unit);
      });
    }

    for (auto &f : futs) {
      std::move(f).get();
    }
  }

  folly::coro::Task<void> coro() override {
    std::vector<folly::coro::TaskWithExecutor<void>> coros;
    for (auto &w : works) {
      coros.emplace_back(w->coro().scheduleOn(ex));
    }

    co_await folly::coro::collectAllRange(std::move(coros));
    co_return;
  }
};

struct SequentialWork : public Work {
  std::vector<std::shared_ptr<Work>> works;
  SequentialWork(std::vector<std::shared_ptr<Work>> &&works,
                 folly::Executor::KeepAlive<> ex)
      : Work(ex), works(std::move(works)) {}

  void sync() override {
    for (auto &w : works) {
      w->sync();
    }
  }

  folly::coro::Task<void> coro() override {
    for (auto &w : works) {
      co_await w->coro();
    }
  }
};

} // namespace playground