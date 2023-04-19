#include "playground/client.h"
#include "folly/experimental/coro/Collect.h"
#include <chrono>
#include <thread>
namespace playground {

folly::coro::Task<void> Client::Pressure(std::function<void()> cob) {
  std::vector<folly::SemiFuture<folly::Unit>> futs;
  for (auto &p : ps) {
    futs.emplace_back(p.getSemiFuture());
    ex.add([p = std::move(p), this, cob]() mutable {
      auto sleepDuration =
          std::chrono::nanoseconds((size_t)(1e9 * o.num_workers / o.qps));
      while (!stop) {
        auto start = std::chrono::system_clock::now();
        cob();
        auto duration = std::chrono::system_clock::now() - start;
        if (duration < sleepDuration) {
          std::this_thread::sleep_for(sleepDuration - duration);
        }
      }
      p.setValue(folly::unit);
    });
  }
  co_await folly::coro::collectAllRange(std::move(futs));
  ex.join();
  co_return;
}

} // namespace playground