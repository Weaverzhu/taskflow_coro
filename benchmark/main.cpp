#include <benchmark/benchmark.h>
#include <folly/init/Init.h>

int main(int argc, char *argv[]) {
  benchmark::Initialize(&argc, argv);
  folly::init(&argc, &argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}