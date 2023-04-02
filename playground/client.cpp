#include "playground/client.h"
namespace playground {

folly::coro::Task<void> Client::Pressure(std::function<void()> &&cob) {

  std::vector<folly::coro::Task<void>> tasks;
  for (int i = 0; i < o.num_workers; ++i) {
    tasks.emplace_back(
        folly::coro::co_invoke([this, cob = cob]() -> folly::coro::Task<void> {
          auto sleepDuration =
              std::chrono::microseconds((int64_t)(1e6 * o.num_workers / o.qps));
          while (!stop) {
            co_await folly::futures::sleep(sleepDuration);
            cob();
          }
          co_return;
        }));
  }

  co_return co_await folly::coro::collectAllRange(std::move(tasks));
}
} // namespace playground