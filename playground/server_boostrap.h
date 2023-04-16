#pragma once

#include <folly/executors/GlobalExecutor.h>
#include <folly/fibers/Semaphore.h>
#include <folly/io/coro/ServerSocket.h>
#include <folly/io/coro/Transport.h>

#include "playground/stats.h"

namespace playground {

struct ServerBoostrap {
  struct Options {
    size_t port;
    folly::EventBase *evb;
    size_t backlog;

    size_t maxConnections;
  };

  ServerBoostrap(Options &&opt = Options())
      : serverSocket(folly::AsyncServerSocket::newSocket(opt.evb),
                     std::make_optional(folly::SocketAddress("::1", opt.port)),
                     opt.backlog),
        metrics(&Metrics::instance()), connectionTokens(opt.maxConnections),
        opt(std::move(opt)) {}

  folly::coro::Task<void>
  Serve(std::function<folly::coro::Task<void>(
            std::unique_ptr<folly::coro::Transport> &&)> &&handler) {
    while (!stop) {
      if (opt.maxConnections) {
        co_await connectionTokens.co_wait();
      }
      auto transport = co_await serverSocket.accept();
      auto fut = folly::coro::co_invoke(handler, std::move(transport))
                     .scheduleOn(folly::getGlobalCPUExecutor())
                     .start();
      if (opt.maxConnections) {
        fut = std::move(fut)
                  .via(&folly::InlineExecutor::instance())
                  .then([this](folly::Try<folly::Unit> &&unit) {
                    this->connectionTokens.signal();
                  });
      }
    }
  }

  folly::coro::ServerSocket serverSocket;
  std::atomic_bool stop{false};
  Metrics *metrics;
  folly::fibers::Semaphore connectionTokens;
  Options opt;
};

} // namespace playground