#include "folly/experimental/coro/Invoke.h"
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Task.h>

namespace taskflow_coro {
struct Work {
  std::function<folly::coro::Task<void>()> coro;
};

struct Node {
  Work w;
  std::vector<folly::SemiFuture<folly::Unit>> preFutures;
  std::vector<folly::Promise<folly::Unit>> sucPromises;

  folly::coro::Task<void> run() {
    co_await folly::coro::collectAllRange(std::move(preFutures));
    co_await folly::coro::co_invoke(w.coro);

    for (auto &p : sucPromises) {
      p.setValue(folly::unit);
    }
  }

  Node &precede(Node &other) {
    sucPromises.push_back(folly::Promise<folly::Unit>());
    other.preFutures.push_back(sucPromises.back().getSemiFuture());
    return *this;
  }
};

struct Taskflow {
  std::list<std::shared_ptr<Node>> nodes;
  Node &emplace_coro(std::function<folly::coro::Task<void>()> &&coro) {
    auto n = std::make_shared<Node>();
    n->w.coro = std::move(coro);

    nodes.push_back(std::move(n));
    return *nodes.back();
  }

  Node &emplace(std::function<void()> &&func) {
    return emplace_coro([f = std::move(func)]() -> folly::coro::Task<void> {
      f();
      co_return;
    });
  }

  folly::coro::Task<void> run() {
    std::vector<folly::coro::Task<void>> tasks;
    tasks.reserve(nodes.size());
    for (auto &n : nodes) {
      tasks.push_back(n->run());
    }
    co_await folly::coro::collectAllRange(std::move(tasks));
    co_return;
  }
};

} // namespace taskflow_coro