#include <folly/stats/TimeseriesHistogram.h>
#include <gtest/gtest.h>

#include <chrono>
#include <random>

#include "playground/stats.h"

using namespace folly;
using namespace playground;
using namespace std::chrono_literals;

class StatsTest : public testing::Test {
public:
  playground::Metrics metrics;
};

TEST_F(StatsTest, test0) {
  static const std::chrono::seconds durations[] = {5s, 10s, 30s};

  folly::TimeseriesHistogram<int64_t> hist(
      10, 0, 10, folly::MultiLevelTimeSeries<int64_t>(10, 3, durations));

  for (int i = 0; i < 10; ++i) {
    hist.addValue(1s, 1);
  }
  hist.update(1s);
  EXPECT_EQ(10, hist.count(0));
}

TEST_F(StatsTest, test1) {

  for (int i = 0; i < 30; ++i) {
    metrics.summary(std::chrono::milliseconds(std::random_device{}() % 10000));
  }
  metrics.flush();
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(metrics.qps.count(i), 30);
    EXPECT_EQ(metrics.qps.avg(i), 1);
  }
}