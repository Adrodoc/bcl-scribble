#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"
#include "DisableableTasLock.cpp"
#include "McsLockBcl.cpp"

class HybridLock : public Lock
{
private:
    DisableableTasLock tas_lock;
    McsLockBcl mcs_lock;

public:
    HybridLock(const HybridLock &) = delete;
    HybridLock(const uint64_t rank = 0)
        : tas_lock{rank},
          mcs_lock{rank}
    {
    }

    ~HybridLock() {}

    void acquire()
    {
        if (tas_lock.try_acquire())
            return;
        if (mcs_lock.acquire_ext())
            tas_lock.disable_try_acquire();
        tas_lock.acquire();
        if (mcs_lock.release_ext())
            tas_lock.enable_try_acquire();
    }

    void release()
    {
        tas_lock.release();
    }
};
