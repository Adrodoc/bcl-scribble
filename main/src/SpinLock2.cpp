#include <bcl_ext/bcl.hpp>

class SpinLock2
{
private:
    BCL::GlobalPtr<BCL::GlobalPtr<int>> flag;

public:
    SpinLock2(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            flag = BCL::alloc<BCL::GlobalPtr<int>>(1);
            *flag = nullptr;
        }
        flag = BCL::broadcast(flag, rank);
    }
    ~SpinLock2() {}
    void acquire()
    {
        while (
            BCL::fetch_and_op(flag, BCL::GlobalPtr<int>(0, 1), BCL::replace<BCL::GlobalPtr<int>>()) != BCL::GlobalPtr<int>(nullptr))
            ;
    }
    void release()
    {
        BCL::fetch_and_op(flag, BCL::GlobalPtr<int>(nullptr), BCL::replace<BCL::GlobalPtr<int>>());
    }
};
