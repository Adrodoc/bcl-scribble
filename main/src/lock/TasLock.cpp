#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"

class TasLock : public Lock
{
private:
    BCL::GlobalPtr<uint8_t> flag;
    int spin_count = 0;
    int acquire_count = 0;

public:
    TasLock(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            flag = BCL::alloc<uint8_t>(1);
            *flag = 0;
        }
        flag = BCL::broadcast(flag, rank);
    }
    ~TasLock()
    {
        BCL::barrier();
        if (flag.is_local())
        {
            BCL::dealloc(flag);
        }
    }

    benchmark::UserCounters counters()
    {
        using namespace benchmark;
        UserCounters counters;
        counters["spin_count"] = Counter(spin_count);
        counters["acquire_count"] = Counter(acquire_count);
        return counters;
    }

    void acquire()
    {
        acquire_count++;
        while (BCL::fetch_and_op(flag, (uint8_t)1, BCL::replace<uint8_t>()))
        {
            BCL::flush();
            spin_count++;
        }
    }
    void release()
    {
        BCL::fetch_and_op(flag, (uint8_t)0, BCL::replace<uint8_t>());
    }
};
