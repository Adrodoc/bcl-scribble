#include <bcl_ext/bcl.hpp>

class SpinLock
{
private:
    BCL::GlobalPtr<int> flag;

public:
    SpinLock(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            flag = BCL::alloc<int>(1);
            *flag = 0;
        }
        flag = BCL::broadcast(flag, rank);
    }
    ~SpinLock() {}
    void acquire()
    {
        while (BCL::fetch_and_op(flag, 1, BCL::replace<int>()))
            ;
    }
    void release()
    {
        BCL::fetch_and_op(flag, 0, BCL::replace<int>());
    }
};
