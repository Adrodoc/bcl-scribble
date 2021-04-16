#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"

class TtsLock : public Lock
{
private:
    BCL::GlobalPtr<uint8_t> flag;

public:
    TtsLock(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            flag = BCL::alloc<uint8_t>(1);
            *flag = 0;
        }
        flag = BCL::broadcast(flag, rank);
    }
    ~TtsLock()
    {
        BCL::barrier();
        if (flag.is_local())
        {
            BCL::dealloc(flag);
        }
    }
    void acquire()
    {
        while (BCL::atomic_rget(flag) || BCL::fetch_and_op(flag, (uint8_t)1, BCL::replace<uint8_t>()))
            BCL::flush();
    }
    void release()
    {
        BCL::fetch_and_op(flag, (uint8_t)0, BCL::replace<uint8_t>());
        BCL::flush();
    }
};
