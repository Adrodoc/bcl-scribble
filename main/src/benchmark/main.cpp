#include "NullReporter.cpp"
#include "benchmarks.cpp"
// #include "lock/DisableableTtsLock.cpp"
#include "lock/HybridLock.cpp"
#include "lock/McsLock.cpp"
// #include "lock/McsLockMpiFlushLocal.cpp"
// #include "lock/McsLockMpiFlushLocalAll.cpp"
// #include "lock/McsLockMpiRequest.cpp"
// #include "lock/TasLock.cpp"
#include "lock/TtsLock.cpp"

void print_processor()
{
  char processor_name_array[MPI_MAX_PROCESSOR_NAME];
  int processor_name_len;
  MPI_Get_processor_name(processor_name_array, &processor_name_len);
  std::string processor_name = std::string{processor_name_array, (std::size_t)processor_name_len};
  std::cout << "(" << BCL::rank() << "/" << BCL::nprocs() << "): Running on processor: "
            << processor_name << std::endl;
}

extern std::string FLAGS_benchmark_out;

int main(int argc, char *argv[])
{
  BCL::init();

  print_processor();
  MPI_Barrier(MPI_COMM_WORLD);

  // REGISTER_LOCK_BENCHMARKS(DisableableTtsLock);
  REGISTER_LOCK_BENCHMARKS(HybridLock);
  REGISTER_LOCK_BENCHMARKS(McsLock);
  // REGISTER_LOCK_BENCHMARKS(McsLockMpiFlushLocal);
  // REGISTER_LOCK_BENCHMARKS(McsLockMpiFlushLocalAll);
  // REGISTER_LOCK_BENCHMARKS(McsLockMpiRequest);
  // REGISTER_LOCK_BENCHMARKS(TasLock);
  REGISTER_LOCK_BENCHMARKS(TtsLock);

  benchmark::Initialize(&argc, argv);
  if (BCL::rank() == 0)
    benchmark::RunSpecifiedBenchmarks();
  else
  {
    // Removing this command line flag to avoid recreating the file when it is already in use.
    FLAGS_benchmark_out = "";
    NullReporter null;
    benchmark::RunSpecifiedBenchmarks(&null);
  }
  BCL::finalize();
  return 0;
}
