#pragma once

#include "playground/server_boostrap.h"

#include <folly/io/Cursor.h>

using namespace std::chrono_literals;

namespace playground {
namespace frozen {

struct EchoServer {

  struct Options : public ServerBoostrap::Options {
    size_t echo_size;
  };

  EchoServer(Options &&options)
      : boostrap(std::move(options)), stop(boostrap.stop) {}

  using Content = std::unique_ptr<folly::IOBuf>;

  struct Handler {
    virtual folly::coro::Task<Content> handle(Content &&req) {
      co_return std::make_unique<folly::IOBuf>();
    }
  };

  folly::coro::Task<void> Serve() {
    co_await boostrap.Serve(
        [this](std::unique_ptr<folly::coro::Transport> &&transport)
            -> folly::coro::Task<void> {
          folly::IOBufQueue queue;

          size_t &frameSize = opt.echo_size;
          co_await transport->read(queue, frameSize, frameSize, 0s);

          co_await transport->write(queue);
          co_return;
        });
  }

  ServerBoostrap boostrap;
  std::atomic_bool &stop;
  Options opt;
};

} // namespace frozen
} // namespace playground
