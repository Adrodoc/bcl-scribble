#include <bcl/bcl.hpp>
#include <mpi_ops.hpp>

class SpinLock
{
private:
    BCL::GlobalPtr<int> flag;

public:
    SpinLock(BCL::GlobalPtr<int> flag) : flag{flag} {}
    ~SpinLock() {}
    void acquire()
    {
        int rank = BCL::rank();
        int nprocs = BCL::nprocs();
        while (BCL::fetch_and_op(flag, 1, BCL::replace<int>()))
            // std::cout << "(" << rank << "/" << nprocs << "): waiting to acquire lock" << std::endl
            ;
    }
    void release()
    {
        int zero = 0;
        BCL::write(&zero, flag, 1);
    }
};
