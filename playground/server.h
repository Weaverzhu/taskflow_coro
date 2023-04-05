#pragma once

#include "playground/server_boostrap.h"

#include <folly/io/Cursor.h>

namespace playground {

using namespace std::chrono_literals;

struct Server {
  Server(ServerBoostrap::Options &&options) : boostrap(std::move(options)) {}

  using Content = std::unique_ptr<folly::IOBuf>;

  struct Handler {
    virtual folly::coro::Task<Content> handle(Content &&req) {
      co_return std::make_unique<folly::IOBuf>();
    }
  };

  folly::coro::Task<void> Serve(std::shared_ptr<Handler> handler) {
    co_await boostrap.Serve(
        [handler = std::move(handler)](
            std::unique_ptr<folly::coro::Transport> &&transport)
            -> folly::coro::Task<void> {
          unsigned char frameSizeBuf[4];
          folly::IOBufQueue queue;
          folly::MutableByteRange range(frameSizeBuf, frameSizeBuf + 4);
          co_await transport->read(range, 0s);
          folly::io::Cursor c(folly::IOBuf::wrapBuffer(range).get());
          size_t frameSize = c.readBE<size_t>();
          co_await transport->read(queue, frameSize, frameSize, 0s);

          auto rsp = co_await handler->handle(queue.move());

          folly::IOBufQueue rspQueue;
          size_t rspSize = rsp->computeChainDataLength();
          auto rspSizeBuf = folly::IOBuf::wrapBuffer(range);
          folly::io::RWCursor<folly::io::CursorAccess::PRIVATE> wc(
              rspSizeBuf.get());

          wc.writeBE(rspSize);
          rspQueue.append(std::move(rspSizeBuf));
          rspQueue.append(std::move(rsp));
          co_await transport->write(rspQueue);
          co_return;
        });
  }

  std::atomic_bool stop{false};

  ServerBoostrap boostrap;
};

} // namespace playground