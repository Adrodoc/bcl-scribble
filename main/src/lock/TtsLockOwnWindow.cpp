#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/mpi_utils.cpp"
#include "mpi_utils/Window.cpp"

class TtsLockOwnWindow : public Lock
{
private:
    enum
    {
        locked_disp = 0, // bool
    };
    const int master_rank;
    const int rank;
    const Window window;

public:
    TtsLockOwnWindow(const TtsLockOwnWindow &) = delete;
    TtsLockOwnWindow(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)((rank == master_rank ? sizeof(bool) : 0)), comm}
    {
        // log() << "entering TtsLockOwnWindow" << std::endl;
        window.lock_all();
        if (rank == master_rank)
            window.set(master_rank, locked_disp, false);
        window.flush_all();
        MPI_Barrier(comm);
        // log() << "exiting TtsLockOwnWindow" << std::endl;
    }

    ~TtsLockOwnWindow()
    {
        // log() << "entering ~TtsLockOwnWindow" << std::endl;
        window.unlock_all();
        // log() << "exiting ~TtsLockOwnWindow" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        while (window.atomic_get<bool>(master_rank, locked_disp) || window.swap(master_rank, locked_disp, true))
            ;
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        window.atomic_set<uint8_t>(master_rank, locked_disp, false);
        // log() << "exiting release()" << std::endl;
    }
};
