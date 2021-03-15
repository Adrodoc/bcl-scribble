#include <bcl/bcl.hpp>
#include <benchmark/benchmark.h>
#include <functional>
#include "Lock.cpp"
#include "McsLock.cpp"
#include "NullReporter.cpp"
#include "SpinLock.cpp"
#include "mpi_benchmark.cpp"

/*
 * The empty-critical-sectionbenchmark (ECSB) derives the throughput of acquiring an empty lock with
 * no workload in the CS.
 */
template <class L>
std::chrono::high_resolution_clock::time_point ecsb(benchmark::State &state, L &lock)
{
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < state.range(); i++)
  {
    lock.acquire();
    lock.release();
  }
  return start;
}

void print_processor()
{
  char processor_name_array[MPI_MAX_PROCESSOR_NAME];
  int processor_name_len;
  MPI_Get_processor_name(processor_name_array, &processor_name_len);
  std::string processor_name = std::string{processor_name_array, (std::size_t)processor_name_len};
  std::cout << "(" << BCL::rank() << "/" << BCL::nprocs() << "): Running on processor: "
            << processor_name << std::endl;
}

int main(int argc, char *argv[])
{
  BCL::init();

  print_processor();
  MPI_Barrier(MPI_COMM_WORLD);

  benchmark::RegisterBenchmark("ecsb/McsLock", mpi_lock_benchmark<McsLock>, ecsb<McsLock>)
      ->UseManualTime()
      ->Range(64, 1 << 15);
  benchmark::RegisterBenchmark("ecsb/SpinLock", mpi_lock_benchmark<SpinLock>, ecsb<SpinLock>)
      ->UseManualTime()
      ->Range(64, 1 << 15);

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
