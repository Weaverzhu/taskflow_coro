#pragma once

#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Invoke.h>
#include <folly/experimental/coro/Task.h>

#include <atomic>

namespace playground {
namespace taskflow_coro {

using R = folly::coro::Task<void>;

struct StaticWork {
  std::function<R()> func;
};

struct Node {
  std::vector<Node *> prev;
  StaticWork work;
  std::atomic_bool hasRun{false};

  R run() {
    if (hasRun.exchange(true)) {
      co_return;
    }

    std::vector<R> tasks;
    tasks.reserve(prev.size());
    for (auto n : prev) {
      tasks.emplace_back(n->run());
    }

    co_await folly::coro::collectAllRange(std::move(tasks));
    co_await folly::coro::co_invoke(work.func);
  }

  void precede(Node &other) { other.prev.push_back(this); }
};

struct Taskflow {
  std::list<std::unique_ptr<Node>> nodes;

  Node &emplace(std::function<void()> &&func) {
    return emplace_coro([func = std::move(func)]() mutable -> R {
      func();
      co_return;
    });
  }

  Node &emplace_coro(std::function<R()> &&coroFunc) {
    auto n = std::make_unique<Node>();
    n->work.func = std::move(coroFunc);
    nodes.emplace_back(std::move(n));
    return *nodes.back().get();
  }

  R run() {
    std::vector<R> tasks;
    tasks.reserve(nodes.size());
    for (auto &n : nodes) {
      tasks.emplace_back(n->run());
    }
    co_await folly::coro::collectAllRange(std::move(tasks));
    co_return;
  }
};

} // namespace taskflow_coro
} // namespace playground