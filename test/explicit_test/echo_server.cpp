#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "playground/handlers/frozen_echo_server.h"

DEFINE_bool(playground_explicit_test_enable_echo_server, false, "");

namespace explicit_test {

TEST(ExplicitTest, echo_server) {
  if (!FLAGS_playground_explicit_test_enable_echo_server) {
    return;
  }
  playground::frozen::EchoServer::Options options{.echo_size = 1024};
  options.backlog = 128;
  options.evb = folly::getGlobalIOExecutor()->getEventBase();
  options.maxConnections = 10;
  options.port = 12345;

  playground::frozen::EchoServer server(std::move(options));

  server.Serve().semi().get();
}

} // namespace explicit_test