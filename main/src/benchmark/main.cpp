#include "NullReporter.cpp"
#include "benchmarks.cpp"
#include "lock/HybridLock.cpp"
#include "lock/Lock.cpp"
#include "lock/McsLock.cpp"
#include "lock/TasLock.cpp"
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

int main(int argc, char *argv[])
{
  BCL::init();

  print_processor();
  MPI_Barrier(MPI_COMM_WORLD);

  REGISTER_LOCK_BENCHMARKS(HybridLock);
  REGISTER_LOCK_BENCHMARKS(McsLock);
  REGISTER_LOCK_BENCHMARKS(TasLock);
  REGISTER_LOCK_BENCHMARKS(TtsLock);

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
