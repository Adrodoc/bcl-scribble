#pragma once

#include <benchmark/benchmark.h>

class Lock
{
public:
    virtual benchmark::UserCounters counters()
    {
        return benchmark::UserCounters{};
    }
    virtual void acquire() = 0;
    virtual void release() = 0;
};
