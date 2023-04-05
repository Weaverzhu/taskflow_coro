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

  auto r = metrics.report();
  EXPECT_NEAR(5, r.qps, 1);
  printf("%d %d\n", (int)r.avg.count(), (int)r.pct99.count());
}

TEST(ClientStatTest, concurrent_test) {
  std::atomic_size_t real_cnt{0};
  folly::TimeseriesHistogram<double> hist(
      10, 0, 100, folly::MultiLevelTimeSeries<double>(10, {100s}));

  std::atomic_bool stop{false};
  folly::Synchronized<std::vector<folly::Future<bool>>> stops;
  for (int i = 0; i < 10; ++i) {
    folly::getGlobalCPUExecutor()->add([&] {
      folly::Promise<bool> p;
      stops.wlock()->push_back(p.getFuture());
      while (!stop) {
        static std::mutex m;
        std::lock_guard<std::mutex> lk(m);
        hist.addValue(std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch()),
                      1);
        real_cnt += 1;
      }
      p.setValue(true);
    });
  }

  std::this_thread::sleep_for(5s);
  stop = true;
  // EXPECT_NEAR(real_cnt, hist.count(0), 1);
  folly::coro::blockingWait(
      folly::coro::collectAllRange(std::move(*stops.wlock())));
}

} // namespace playground
