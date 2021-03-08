#include <benchmark/benchmark.h>

// This reporter does nothing.
// We can use it to disable output from all but the root process.
class NullReporter : public benchmark::BenchmarkReporter
{
public:
    NullReporter() {}
    virtual bool ReportContext(const Context &) { return true; }
    virtual void ReportRuns(const std::vector<Run> &) {}
    virtual void Finalize() {}
};
