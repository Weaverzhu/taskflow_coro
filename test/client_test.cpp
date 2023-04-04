#include "playground/client.h"
#include "playground/stats.h"

#include <folly/experimental/coro/BlockingWait.h>
#include <gtest/gtest.h>

#include <random>
namespace playground {

using namespace std::chrono_literals;
TEST(ClientStatTest, test0) {
  Client client(Client::Options{.num_workers = 1, .qps = 5});
  Metrics metrics;

  struct Context {

    std::default_random_engine engine;
    std::uniform_int_distribution<int> distribution =
        std::uniform_int_distribution<int>(1, 100);
  };
  Context ctx;
  auto task = client
                  .Pressure([&]() mutable {
                    // static std::mutex m;
                    // std::lock_guard<std::mutex> lk(m);
                    metrics.summary(std::chrono::milliseconds(
                        ctx.distribution(ctx.engine)));
                  })
                  .scheduleOn(folly::getGlobalCPUExecutor())
                  .start();

  std::this_thread::sleep_for(std::chrono::seconds(10));
  client.stop = true;

  std::move(task).get();

  for (int i = 0; i <= 100; ++i) {
    std::cout << i << ' ' << metrics.latency.getPercentileEstimate(i, 0)
              << metrics.qps.getPercentileEstimate(i, 0) << std::endl;
  }
}
} // namespace playground
