#include "playground/stats.h"

#include <folly/stats/MultiLevelTimeSeries.h>

namespace playground {

using namespace std::chrono;
using namespace folly;
using namespace std::chrono_literals;

static const seconds kDurations[] = {1s, 5s, 30s};
static const size_t kNumShards = 10;

Metrics::Metrics()
    : base(system_clock::now()),
      qps(kNumShards, 1, 2,
          folly::MultiLevelTimeSeries<double>(
              kNumShards, sizeof(kDurations) / sizeof(kDurations[0]),
              kDurations)),
      latency(kNumShards, 0, 1000000,
              folly::MultiLevelTimeSeries<double>(
                  kNumShards, sizeof(kDurations) / sizeof(kDurations[0]),
                  kDurations)) {}

void Metrics::summary(Metrics::Clock &&lat) {
  std::lock_guard<std::mutex> lk(summary_mutex);

  auto n = duration_cast<seconds>(system_clock::now().time_since_epoch());
  qps.addValue(n, 1);
  latency.addValue(n, lat.count());
}

} // namespace playground