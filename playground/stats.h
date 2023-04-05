#pragma once

#include <chrono>

#include <folly/stats/BucketedTimeSeries.h>
#include <folly/stats/TimeseriesHistogram.h>

namespace playground {
struct Metrics {
  using TP = std::chrono::system_clock::time_point;
  using Clock = std::chrono::microseconds;

  struct Report {
    double qps;
    std::chrono::system_clock::duration avg, pct99;
  };

  Metrics();

  void summary(Clock &&latency);

  void flush() {
    qps.update(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()));
    latency.update(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()));
  }

  void clear() {
    qps.clear();
    latency.clear();
  }

  Report report(size_t level = 0) {
    return Report{.qps = qps.rate(level),
                  .avg = std::chrono::microseconds((size_t)latency.avg(level)),
                  .pct99 = std::chrono::microseconds(
                      (size_t)latency.getPercentileEstimate(99, level))};
  }

  static Metrics &instance() {
    static Metrics ins;
    return ins;
  }

  void dump(size_t level) {
    printf("throughput: %d, pct99 latency: %dus\n", (int)qps.rate(level),
           (int)latency.getPercentileEstimate(99, level));
  }

  TP base;

  folly::TimeseriesHistogram<int64_t> qps, latency;
};
} // namespace playground