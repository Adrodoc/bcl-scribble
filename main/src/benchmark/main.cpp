#include <bcl/bcl.hpp>
#include <benchmark/benchmark.h>
#include "McsLock.cpp"
#include "SpinLock.cpp"
#include "NullReporter.cpp"
#include "mpi_benchmark.cpp"

std::chrono::high_resolution_clock::time_point benchmark_McsLock(benchmark::State &state)
{
  McsLock lock;
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < 10; i++)
  {
    lock.acquire();
    lock.release();
  }
  return start;
}

std::chrono::high_resolution_clock::time_point benchmark_SpinLock(benchmark::State &state)
{
  SpinLock lock;
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < 10; i++)
  {
    lock.acquire();
    lock.release();
  }
  return start;
}

int main(int argc, char *argv[])
{
  BCL::init();

  benchmark::RegisterBenchmark("benchmark_McsLock", mpi_benchmark, benchmark_McsLock)->UseManualTime();
  benchmark::RegisterBenchmark("benchmark_SpinLock", mpi_benchmark, benchmark_SpinLock)->UseManualTime();

  benchmark::Initialize(&argc, argv);
  if (BCL::rank() == 0)
    benchmark::RunSpecifiedBenchmarks();
  else
  {
    NullReporter null;
    benchmark::RunSpecifiedBenchmarks(&null);
  }
  BCL::finalize();
  return 0;
}
