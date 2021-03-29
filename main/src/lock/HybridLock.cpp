#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"
#include "DisableableTtsLock.cpp"
#include "McsLock.cpp"

class HybridLock : public Lock
{
private:
    DisableableTtsLock tts_lock;
    McsLock mcs_lock;

public:
    HybridLock(const HybridLock &) = delete;
    HybridLock(const uint64_t rank = 0)
        : tts_lock{rank},
          mcs_lock{rank}
    {
    }

    ~HybridLock() {}

    void acquire()
    {
        if (tts_lock.try_acquire())
            return;
        if (mcs_lock.acquire_ext())
            tts_lock.disable_try_acquire();
        tts_lock.acquire();
        if (mcs_lock.release_ext())
            tts_lock.enable_try_acquire();
    }

    void release()
    {
        tts_lock.release();
    }
};
