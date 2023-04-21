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
[==========] 1 test from 1 test suite ran. (3836 ms total)
[  PASSED  ] 1 test.

 Performance counter stats for 'xmake run test --gtest_filter=coro_work.sync1':

         58,626.33 msec task-clock                #   15.018 CPUs utilized          
         4,657,671      context-switches          #   79.447 K/sec                  
           110,264      cpu-migrations            #    1.881 K/sec                  
            26,123      page-faults               #  445.585 /sec                   
   243,008,828,685      cycles                    #    4.145 GHz                      (62.51%)
     3,504,540,935      stalled-cycles-frontend   #    1.44% frontend cycles idle     (62.76%)
    12,217,609,496      stalled-cycles-backend    #    5.03% backend cycles idle      (63.03%)
   101,760,148,890      instructions              #    0.42  insn per cycle         
                                                  #    0.12  stalled cycles per insn  (63.02%)
    18,805,789,014      branches                  #  320.774 M/sec                    (62.93%)
       385,603,806      branch-misses             #    2.05% of all branches          (62.57%)
    35,940,812,394      L1-dcache-loads           #  613.049 M/sec                    (62.31%)
     2,609,811,853      L1-dcache-load-misses     #    7.26% of all L1-dcache accesses  (62.35%)
   <not supported>      LLC-loads                                                   
   <not supported>      LLC-load-misses                                             

       3.903706047 seconds time elapsed

      10.146664000 seconds user
      48.468208000 seconds sys

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
[==========] 1 test from 1 test suite ran. (3684 ms total)
[  PASSED  ] 1 test.

 Performance counter stats for 'xmake run test --gtest_filter=coro_work.coro1':

         40,584.19 msec task-clock                #   10.709 CPUs utilized          
         3,502,792      context-switches          #   86.309 K/sec                  
            28,912      cpu-migrations            #  712.396 /sec                   
             7,036      page-faults               #  173.368 /sec                   
   155,403,397,316      cycles                    #    3.829 GHz                      (62.51%)
     2,681,060,827      stalled-cycles-frontend   #    1.73% frontend cycles idle     (62.04%)
     7,345,562,231      stalled-cycles-backend    #    4.73% backend cycles idle      (62.48%)
    74,192,495,785      instructions              #    0.48  insn per cycle         
                                                  #    0.10  stalled cycles per insn  (62.53%)
    13,308,013,070      branches                  #  327.911 M/sec                    (63.17%)
       339,798,675      branch-misses             #    2.55% of all branches          (62.52%)
    27,765,782,420      L1-dcache-loads           #  684.153 M/sec                    (62.52%)
     1,603,714,327      L1-dcache-load-misses     #    5.78% of all L1-dcache accesses  (62.49%)
   <not supported>      LLC-loads                                                   
   <not supported>      LLC-load-misses                                             

       3.789579743 seconds time elapsed

       7.113134000 seconds user
      33.095524000 seconds sys

*/
// clang-format on

TEST(coro_work, coro1) {
  folly::CPUThreadPoolExecutor ex(std::thread::hardware_concurrency());
  auto w = compose_work(&ex);
  w->coro().scheduleOn(&ex).start().get();
}

} // namespace playground_test::coro::work