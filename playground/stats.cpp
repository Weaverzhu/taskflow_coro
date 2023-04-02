#include "playground/stats.h"

#include <folly/stats/MultiLevelTimeSeries.h>

namespace playground {

using namespace std::chrono;
using namespace folly;
using namespace std::chrono_literals;

static const seconds kDurations[] = {5s, 30s, 300s};

Metrics::Metrics()
    : base(system_clock::now()),
      qps(10, 1, 2, folly::MultiLevelTimeSeries<int64_t>(10, 3, kDurations)),
      latency(10, 0, 1000000,
              folly::MultiLevelTimeSeries<int64_t>(10, 3, kDurations)) {}

void Metrics::summary(Metrics::Clock &&lat) {
  auto n = duration_cast<seconds>(system_clock::now().time_since_epoch());
  qps.addValue(n, 1);
  latency.addValue(n, lat.count());
}

} // namespace playground