#include <folly/experimental/coro/Task.h>
#include <gtest/gtest.h>

static folly::coro::Task<void> foo() {
  puts("foo");
  co_return;
}

folly::Future<folly::Unit> bar(folly::Unit &&) { return folly::unit; }

void some_regular_func() {
  // 1. this is ok
  foo().semi().via(&folly::InlineExecutor::instance()).thenValue(bar).get();
}

TEST(ExecutorCoroTest, test0) {
  // 2. this is not ok
  // foo().scheduleOn(&folly::InlineExecutor::instance()).start().get();

  some_regular_func();
}