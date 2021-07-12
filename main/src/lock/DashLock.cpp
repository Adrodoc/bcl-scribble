#include <libdash.h>
#include "Lock.cpp"
#include "log.cpp"

class DashLock : public Lock
{
private:
    dash::Mutex mx;

public:
    DashLock(const DashLock &) = delete;
    DashLock(MPI_Comm comm = MPI_COMM_WORLD)
    {
        // log() << "entering DashLock()" << std::endl;
        // log() << "exiting DashLock()" << std::endl;
    }

    ~DashLock()
    {
        // log() << "entering ~DashLock()" << std::endl;
        // log() << "exiting ~DashLock()" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        mx.lock();
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        mx.unlock();
        // log() << "exiting release()" << std::endl;
    }
};
