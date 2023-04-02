#pragma once

#include <chrono>

#include <folly/stats/BucketedTimeSeries.h>
#include <folly/stats/TimeseriesHistogram.h>

namespace playground {
struct Metrics {
  using TP = std::chrono::system_clock::time_point;
  using Clock = std::chrono::microseconds;

  Metrics();

  void summary(Clock &&latency);

  void flush() {
    qps.update(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()));
    latency.update(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()));
  }

  TP base;

  folly::TimeseriesHistogram<int64_t> qps, latency;
};
} // namespace playground