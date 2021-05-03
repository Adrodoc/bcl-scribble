#include <benchmark/benchmark.h>

template <class L>
void mpi_lock_benchmark(
    benchmark::State &state,
    std::function<std::chrono::duration<double>(benchmark::State &, L &)> benchmark)
{
    mpi_lock_benchmark_with_supplier(state, benchmark, std::make_unique<L>);
}

void Increment(benchmark::UserCounters *l, benchmark::UserCounters const &r);

template <class L>
void mpi_lock_benchmark_with_supplier(
    benchmark::State &state,
    std::function<std::chrono::duration<double>(benchmark::State &, L &)> benchmark,
    std::function<std::unique_ptr<L>()> lock_supplier)
{
    auto lock = lock_supplier();

    // warm up
    for (int i = 0; i < 5; i++)
        benchmark(state, *lock);

    for (auto _ : state)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        // Do the work and time it on each proc
        auto const duration = benchmark(state, *lock);
        // Now get the max time across all procs:
        // for better or for worse, the slowest processor is the one that is
        // holding back the others in the benchmark.
        double const elapsed_seconds = duration.count();
        double max_elapsed_second;
        MPI_Allreduce(&elapsed_seconds, &max_elapsed_second, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        state.SetIterationTime(max_elapsed_second);
        Increment(&state.counters, lock->counters());
    }
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int64_t critical_sections = state.iterations() * state.range() * size;
    using Counter = benchmark::Counter;
    state.counters["critical_sections"] = Counter(critical_sections, Counter::kIsRate);
    state.SetLabel("{\"mpi_processes\":" + std::to_string(size) + "}");
}

void Increment(benchmark::UserCounters *l, benchmark::UserCounters const &r)
{
    // add counters present in both or just in *l
    for (auto &c : *l)
    {
        auto it = r.find(c.first);
        if (it != r.end())
        {
            c.second.value = c.second + it->second;
        }
    }
    // add counters present in r, but not in *l
    for (auto const &tc : r)
    {
        auto it = l->find(tc.first);
        if (it == l->end())
        {
            (*l)[tc.first] = tc.second;
        }
    }
}
