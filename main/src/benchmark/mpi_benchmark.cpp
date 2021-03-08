#include <benchmark/benchmark.h>

void mpi_benchmark(
    benchmark::State &state,
    std::chrono::high_resolution_clock::time_point (*benchmark)(benchmark::State &))
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int i = 0;
    for (auto _ : state)
    {
        McsLock lock;
        // Do the work and time it on each proc
        auto start = benchmark(state);
        MPI_Barrier(MPI_COMM_WORLD);
        auto end = std::chrono::high_resolution_clock::now();
        // Now get the max time across all procs:
        // for better or for worse, the slowest processor is the one that is
        // holding back the others in the benchmark.
        auto const duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        auto elapsed_seconds = duration.count();
        double max_elapsed_second;
        MPI_Allreduce(&elapsed_seconds, &max_elapsed_second, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        state.SetIterationTime(max_elapsed_second);
    }
}
