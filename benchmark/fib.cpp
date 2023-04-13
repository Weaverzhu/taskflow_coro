#include "folly/executors/GlobalExecutor.h"
#include <benchmark/benchmark.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Task.h>
#include <taskflow/taskflow.hpp>

uint32_t cpuTask() {
  std::string s(200, 'x');
  uint32_t ret = 0;
  for (int i = 0; i < 200; ++i) {
    ret += std::hash<std::string>{}(s);
  }
  return ret;
}

int fib_dynamic(int n, tf::Subflow &flow) {
  if (n <= 2) {
    return n;
  }
  int res1 = 0, res2 = 0;
  auto n1 = flow.emplace(
      [&res1, n](tf::Subflow &flow) { res1 = fib_dynamic(n - 1, flow); });
  auto n2 = flow.emplace(
      [&res2, n](tf::Subflow &flow) { res2 = fib_dynamic(n - 2, flow); });
  flow.join();

  cpuTask();
  return res1 + res2;
}

void fib_taskflow(int n) {
  static tf::Executor ex;
  tf::Taskflow flow;
  flow.emplace([n](tf::Subflow &sf) { fib_dynamic(n, sf); });
  ex.run(flow).wait();
}

void BM_fib_taskflow(benchmark::State &state) {
  tf::Taskflow tf;
  for (auto _ : state) {
    fib_taskflow(state.range(0));
  }
}

BENCHMARK(BM_fib_taskflow)->Arg(15)->Arg(17)->Arg(20);

folly::coro::Task<int> fib_coro(int n) {
  if (n <= 2) {
    co_return n;
  }
  auto [res1, res2] = co_await folly::coro::collectAll(
      fib_coro(n - 1).scheduleOn(folly::getGlobalCPUExecutor()).start(),
      fib_coro(n - 2).scheduleOn(folly::getGlobalCPUExecutor()).start());
  cpuTask();
  co_return res1 + res2;
}

void BM_fib_coro(benchmark::State &state) {
  tf::Taskflow tf;
  for (auto _ : state) {
    fib_coro(state.range(0))
        .scheduleOn(folly::getGlobalCPUExecutor())
        .start()
        .get();
  }
}

BENCHMARK(BM_fib_coro)->Arg(15)->Arg(17)->Arg(20);