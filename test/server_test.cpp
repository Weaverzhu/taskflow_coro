#include "playground/server_boostrap.h"

#include <folly/experimental/coro/BlockingWait.h>
#include <folly/io/Cursor.h>
#include <gtest/gtest.h>

#include <chrono>

using namespace std::chrono_literals;

folly::coro::Task<void> echo(size_t port) {
  auto transport = co_await folly::coro::Transport::newConnectedSocket(
      folly::getGlobalIOExecutor()->getEventBase(),
      folly::SocketAddress("::1", port), 0s);
  auto buf = folly::IOBuf::create(4);
  folly::io::RWCursor<folly::io::CursorAccess::PRIVATE> c(buf.get());
  size_t size = 1234;
  c.writeBE(size);

  folly::IOBufQueue queue;
  queue.append(std::move(buf));

  buf = folly::IOBuf::create(size);
  memset(buf->writableData(), 'x', size);
  queue.append(std::move(buf));

  co_await transport.write(queue, 0s);

  folly::IOBufQueue readQueue;
  co_await transport.read(readQueue, size, size, 0s);

  auto bufRead = readQueue.move();
  for (int i = 0; i < size; ++i) {
    EXPECT_EQ(bufRead->writableData()[i], buf->writableData()[i]);
  }
}

TEST(ServerTest, test0) {
  playground::ServerBoostrap server(playground::ServerBoostrap::Options{
      .port = 12345,
      .evb = folly::getGlobalIOExecutor()->getEventBase(),
      .backlog = 128,
      .maxConnections = 0,
  });

  auto fut =
      server
          .Serve([](std::unique_ptr<folly::coro::Transport> &&transport)
                     -> folly::coro::Task<void> {
            unsigned char frameSizeBuf[4];
            folly::IOBufQueue queue;
            folly::MutableByteRange range(frameSizeBuf, frameSizeBuf + 4);
            co_await transport->read(range, 0s);
            folly::io::Cursor c(folly::IOBuf::wrapBuffer(range).get());
            size_t frameSize = c.readBE<size_t>();
            co_await transport->read(queue, frameSize, frameSize, 0s);

            co_await transport->write(queue);
            co_return;
          })
          .semi();
  auto fut2 = echo(12345).scheduleOn(folly::getGlobalCPUExecutor()).start();
  std::this_thread::sleep_for(5s);

  server.stop = true;
  std::move(fut).get();
}