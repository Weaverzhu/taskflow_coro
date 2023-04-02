#include "playground/client.h"
#include "playground/stats.h"

#include <gtest/gtest.h>

namespace playground {

TEST(ClientStatTest, test0) {
  Client client(Client::Options{.num_workers = 1, .qps = 123});
  Metrics metrics;
}
} // namespace playground
