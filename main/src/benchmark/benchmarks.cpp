#include <bcl_ext/bcl.hpp>
#include <random>
#include "mpi_benchmark.cpp"

/*
 * The empty-critical-section benchmark (ECSB) derives the throughput of acquiring an empty lock
 * with no workload in the CS.
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
    std::random_device rd;                      // non-deterministic generator
    std::mt19937 gen(rd());                     // to seed mersenne twister.
    std::uniform_int_distribution<> dist(1, 4); // distribute results between 1 and 4 inclusive.
    auto counter = BCL::alloc_shared<int>(1);
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < state.range(); i++)
    {
        lock.acquire();
        BCL::rput(dist(gen), counter);
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

/*
 * The wait-after-release benchmark (WARB) varies lock contention: after release, processes wait for
 * a random time (1-4μs) before the next acquire.
 */
template <class L>
std::chrono::duration<double> warb(benchmark::State &state, L &lock)
{
    std::random_device rd;                      // non-deterministic generator
    std::mt19937 gen(rd());                     // to seed mersenne twister.
    std::uniform_int_distribution<> dist(1, 4); // distribute results between 1 and 4 inclusive.
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < state.range(); i++)
    {
        lock.acquire();
        lock.release();
        auto started_spinning = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds time_to_wait{dist(gen)};
        auto spin_until = started_spinning + time_to_wait;
        while (std::chrono::high_resolution_clock::now() < spin_until)
            ;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return end - start;
}

/*
 * The wait benchmark (WB) is a sanity check: processes wait for a random time (1-4μs) without
 * involving a lock.
 */
template <class L>
std::chrono::duration<double> wb(benchmark::State &state, L &lock)
{
    std::random_device rd;                      // non-deterministic generator
    std::mt19937 gen(rd());                     // to seed mersenne twister.
    std::uniform_int_distribution<> dist(1, 4); // distribute results between 1 and 4 inclusive.
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < state.range(); i++)
    {
        auto started_spinning = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds time_to_wait{dist(gen)};
        auto spin_until = started_spinning + time_to_wait;
        while (std::chrono::high_resolution_clock::now() < spin_until)
            ;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return end - start;
}

template <class L>
void registerLockBenchmark(const std::string name, std::function<std::chrono::duration<double>(benchmark::State &, L &)> benchmark)
{
    benchmark::RegisterBenchmark(name.c_str(), mpi_lock_benchmark<L>, benchmark)
        ->UseManualTime()
        ->Arg(1 << 8);
}

#define REGISTER_LOCK_BENCHMARK(b, L) registerLockBenchmark<L>(#b + std::string("/") + #L, b<L>);

#define REGISTER_LOCK_BENCHMARKS(L)   \
    REGISTER_LOCK_BENCHMARK(ecsb, L); \
    REGISTER_LOCK_BENCHMARK(sob, L);  \
    REGISTER_LOCK_BENCHMARK(wcsb, L); \
    REGISTER_LOCK_BENCHMARK(warb, L);
