#include "playground/client.h"
namespace playground {

folly::coro::Task<void> Client::Pressure(folly::Func cob) {

  co_await folly::coro::co_invoke(
      [this, cob = std::move(cob)]() mutable -> folly::coro::Task<void> {
        auto sleepDuration = std::chrono::microseconds((int64_t)(1e6 / o.qps));
        while (!stop) {
          co_await folly::futures::sleep(sleepDuration);
          cob();
        }
        co_return;
      });
  co_return;
}

} // namespace playground