#include <folly/executors/CPUThreadPoolExecutor.h>
#include <gtest/gtest.h>

#include <random>

#include "playground/work.h"

namespace playground_test::coro::work {
using namespace playground;

std::shared_ptr<Work>
compose_work(folly::Executor::KeepAlive<> ex = folly::getGlobalCPUExecutor(),
             int fanout_size = 180, int seq_size = 2, int seed = 3306) {
  std::vector<std::shared_ptr<Work>> fanout;
  std::default_random_engine engine(seed);

  std::uniform_int_distribution<size_t> work_distribution(0, 1);
  std::uniform_int_distribution<size_t> io_distribution(30, 300);
  std::uniform_int_distribution<size_t> mem_cpu_distribution(100000, 200000);

  for (int i = 0; i < fanout_size; ++i) {
    if (work_distribution(engine) == 0) {
      fanout.emplace_back(
          std::make_shared<MemCPUWork>(mem_cpu_distribution(engine), ex));
    } else {
      fanout.emplace_back(
          std::make_shared<IOWork>(io_distribution(engine), ex));
    }
  }

  std::vector<std::shared_ptr<Work>> seq;
  seq.reserve(seq_size);
  for (int i = 0; i < seq_size; ++i) {
    auto fanout_copy = fanout;
    seq.emplace_back(std::make_shared<FanoutWork>(std::move(fanout_copy), ex));
  }

  return std::make_shared<SequentialWork>(std::move(seq), ex);
}

// clang-format off
/*
[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (3699 ms total)
[  PASSED  ] 1 test.

 Performance counter stats for 'xmake run test --gtest_filter=coro_work.sync1':

         58,259.08 msec task-clock                #   15.463 CPUs utilized          
         4,979,076      context-switches          #   85.464 K/sec                  
            32,916      cpu-migrations            #  564.994 /sec                   
            26,207      page-faults               #  449.835 /sec                   
   240,334,646,317      cycles                    #    4.125 GHz                      (62.70%)
     3,786,664,642      stalled-cycles-frontend   #    1.58% frontend cycles idle     (62.59%)
    11,628,007,498      stalled-cycles-backend    #    4.84% backend cycles idle      (62.65%)
    88,210,404,530      instructions              #    0.37  insn per cycle         
                                                  #    0.13  stalled cycles per insn  (62.79%)
    16,903,456,440      branches                  #  290.143 M/sec                    (62.96%)
       381,161,479      branch-misses             #    2.25% of all branches          (62.67%)
    30,010,244,892      L1-dcache-loads           #  515.117 M/sec                    (62.54%)
     2,654,035,140      L1-dcache-load-misses     #    8.84% of all L1-dcache accesses  (62.51%)
   <not supported>      LLC-loads                                                   
   <not supported>      LLC-load-misses                                             

       3.767736047 seconds time elapsed

       7.288427000 seconds user
      50.963515000 seconds sys
*/
// clang-format on

TEST(coro_work, sync1) {
  folly::CPUThreadPoolExecutor ex(1000);
  auto w = compose_work(&ex);
  w->sync();
}

// clang-format off
/*
[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (2758 ms total)
[  PASSED  ] 1 test.

 Performance counter stats for 'xmake run test --gtest_filter=coro_work.coro1':

         28,700.28 msec task-clock                #    9.977 CPUs utilized          
         2,711,839      context-switches          #   94.488 K/sec                  
            24,040      cpu-migrations            #  837.622 /sec                   
             7,013      page-faults               #  244.353 /sec                   
   108,050,706,724      cycles                    #    3.765 GHz                      (62.38%)
     2,072,788,448      stalled-cycles-frontend   #    1.92% frontend cycles idle     (62.06%)
     5,050,812,844      stalled-cycles-backend    #    4.67% backend cycles idle      (63.19%)
    44,921,881,948      instructions              #    0.42  insn per cycle         
                                                  #    0.11  stalled cycles per insn  (62.92%)
     8,397,287,199      branches                  #  292.585 M/sec                    (62.85%)
       254,965,431      branch-misses             #    3.04% of all branches          (62.21%)
    16,113,117,976      L1-dcache-loads           #  561.427 M/sec                    (62.62%)
     1,220,365,475      L1-dcache-load-misses     #    7.57% of all L1-dcache accesses  (62.17%)
   <not supported>      LLC-loads                                                   
   <not supported>      LLC-load-misses                                             

       2.876658572 seconds time elapsed

       3.677696000 seconds user
      24.693447000 seconds sys
*/
// clang-format on

TEST(coro_work, coro1) {
  folly::CPUThreadPoolExecutor ex(std::thread::hardware_concurrency());
  auto w = compose_work(&ex);
  w->coro().scheduleOn(&ex).start().get();
}

} // namespace playground_test::coro::work