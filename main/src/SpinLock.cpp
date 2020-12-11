#include <bcl_ext/bcl.hpp>

class SpinLock
{
private:
    BCL::GlobalPtr<int> flag;

public:
    SpinLock(BCL::GlobalPtr<int> flag) : flag{flag} {}
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
