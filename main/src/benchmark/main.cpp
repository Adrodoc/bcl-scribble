#include <bcl/bcl.hpp>
#include <benchmark/benchmark.h>
#include <functional>
#include <random>
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
std::chrono::duration<double> ecsb(benchmark::State &state, L &lock)
{
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < state.range(); i++)
  {
    lock.acquire();
    lock.release();
  }
  auto end = std::chrono::high_resolution_clock::now();
  return end - start;
}

/*
 * The single-operation benchmark (SOB) measures the throughput of acquiring a lock with only one
 * single operation (one memory access) in the CS; it represents irregular parallel work-loads such
 * as graph processing with vertices protected by fine locks.
 */
template <class L>
std::chrono::duration<double> sob(benchmark::State &state, L &lock)
{
  auto counter = BCL::alloc_shared<int>(1);
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < state.range(); i++)
  {
    lock.acquire();
    BCL::rput((int)i, counter);
    lock.release();
  }
  auto end = std::chrono::high_resolution_clock::now();
  return end - start;
}

/*
 * The workload-critical-section benchmark (WCSB) covers variable workloads in the CS: each process
 * increments a shared counter and then spins for a random time (1-4μs) to simulate local
 * computation.
 */
template <class L>
std::chrono::duration<double> wcsb(benchmark::State &state, L &lock)
{
  std::random_device rd;                      // non-deterministic generator
  std::mt19937 gen(rd());                     // to seed mersenne twister.
  std::uniform_int_distribution<> dist(1, 4); // distribute results between 1 and 4 inclusive.
  auto counter = BCL::alloc_shared<int>(1);
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < state.range(); i++)
  {
    lock.acquire();
    int c = BCL::rget(counter) + 1;
    BCL::rput(c, counter);
    auto started_spinning = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds time_to_wait{dist(gen)};
    auto spin_until = started_spinning + time_to_wait;
    while (std::chrono::high_resolution_clock::now() < spin_until)
      ;
    lock.release();
  }
  auto end = std::chrono::high_resolution_clock::now();
  return end - start;
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
      ->Arg(1 << 10);
  benchmark::RegisterBenchmark("ecsb/SpinLock", mpi_lock_benchmark<SpinLock>, ecsb<SpinLock>)
      ->UseManualTime()
      ->Arg(1 << 10);
  benchmark::RegisterBenchmark("sob/McsLock", mpi_lock_benchmark<McsLock>, sob<McsLock>)
      ->UseManualTime()
      ->Arg(1 << 10);
  benchmark::RegisterBenchmark("sob/SpinLock", mpi_lock_benchmark<SpinLock>, sob<SpinLock>)
      ->UseManualTime()
      ->Arg(1 << 10);
  benchmark::RegisterBenchmark("wcsb/McsLock", mpi_lock_benchmark<McsLock>, wcsb<McsLock>)
      ->UseManualTime()
      ->Arg(1 << 10);
  benchmark::RegisterBenchmark("wcsb/SpinLock", mpi_lock_benchmark<SpinLock>, wcsb<SpinLock>)
      ->UseManualTime()
      ->Arg(1 << 10);

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
