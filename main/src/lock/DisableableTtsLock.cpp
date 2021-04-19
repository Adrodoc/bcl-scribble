#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"

class DisableableTtsLock : public Lock
{
private:
    const uint8_t LOCKED_BIT = 1;
    const uint8_t ENABLED_BIT = 1 << 1;
    const uint8_t ENABLED_UNLOCKED = ENABLED_BIT;
    const uint8_t ENABLED_LOCKED = ENABLED_BIT | LOCKED_BIT;
    BCL::GlobalPtr<uint8_t> flag;

public:
    DisableableTtsLock(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            flag = BCL::alloc<uint8_t>(1);
            BCL::rput((uint8_t)0, flag);
        }
        flag = BCL::broadcast(flag, rank);
    }
    ~DisableableTtsLock()
    {
        BCL::barrier();
        if (flag.is_local())
        {
            BCL::dealloc(flag);
        }
    }
    void acquire()
    {
        while ((BCL::atomic_rget(flag) & LOCKED_BIT) ||                                 // test LOCKED_BIT
               (BCL::fetch_and_op(flag, LOCKED_BIT, BCL::bor<uint8_t>()) & LOCKED_BIT)) // test and set LOCKED_BIT
            BCL::flush();
    }
    void release()
    {
        BCL::op(flag, (uint8_t)~LOCKED_BIT, BCL::band<uint8_t>()); // unset LOCKED_BIT
    }
    bool try_acquire()
    {
        return BCL::atomic_rget(flag) == ENABLED_UNLOCKED && BCL::compare_and_swap(flag, ENABLED_UNLOCKED, ENABLED_LOCKED) == ENABLED_UNLOCKED;
    }
    void enable_try_acquire()
    {
        BCL::op(flag, ENABLED_BIT, BCL::bor<uint8_t>()); // set ENABLED_BIT
    }
    void disable_try_acquire()
    {
        BCL::op(flag, (uint8_t)~ENABLED_BIT, BCL::band<uint8_t>()); // unset ENABLED_BIT
    }
};
