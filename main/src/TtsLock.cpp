#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"

class TtsLock : public Lock
{
private:
    BCL::GlobalPtr<int> flag;

public:
    TtsLock(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            flag = BCL::alloc<int>(1);
            *flag = 0;
        }
        flag = BCL::broadcast(flag, rank);
    }
    ~TtsLock() {}
    void acquire()
    {
        while (BCL::atomic_rget(flag) || BCL::fetch_and_op(flag, 1, BCL::replace<int>()))
            ;
    }
    void release()
    {
        BCL::fetch_and_op(flag, 0, BCL::replace<int>());
    }
};
