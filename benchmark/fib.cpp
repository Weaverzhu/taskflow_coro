#include "folly/executors/GlobalExecutor.h"
#include "folly/executors/InlineExecutor.h"
#include <benchmark/benchmark.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Task.h>
#include <taskflow/taskflow.hpp>

uint32_t cpuTask() {
  std::string s(200, 'x');
  uint32_t ret = 0;
  for (int i = 0; i < 500; ++i) {
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

BENCHMARK(BM_fib_taskflow)->Arg(15)->Arg(17)->Arg(20)->Arg(22);

folly::coro::Task<int> fib_coro(int n) {
  if (n <= 2) {
    co_return n;
  }
  auto [res1, res2] =
      co_await folly::coro::collectAll(
          fib_coro(n - 1).scheduleOn(folly::getGlobalCPUExecutor()),
          fib_coro(n - 2))
          .scheduleOn(&folly::InlineExecutor::instance());
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

BENCHMARK(BM_fib_coro)->Arg(15)->Arg(17)->Arg(20)->Arg(22);

/*
-------------------------------------------------------------
Benchmark                   Time             CPU   Iterations
-------------------------------------------------------------
BM_fib_taskflow/15     938273 ns        49444 ns        10000
BM_fib_taskflow/17    2214607 ns       118198 ns         5890
BM_fib_taskflow/20    8663702 ns       518081 ns         1000
BM_fib_taskflow/22   22067101 ns      1444260 ns          487
BM_fib_coro/15         835111 ns         2351 ns        10000
BM_fib_coro/17        2015837 ns         2507 ns        10000
BM_fib_coro/20        8212625 ns         2715 ns         1000
BM_fib_coro/22       21384419 ns         4053 ns         1000
*/