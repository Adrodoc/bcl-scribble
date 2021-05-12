#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/Window.cpp"

class TtsLockOwnWindow : public Lock
{
private:
    enum
    {
        locked_disp = 0, // uint8_t
    };
    const int master_rank;
    const int rank;
    const Window window;

    static int get_rank(const MPI_Comm comm)
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        return rank;
    }

public:
    TtsLockOwnWindow(const TtsLockOwnWindow &) = delete;
    TtsLockOwnWindow(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)((rank == master_rank ? 1 : 0)), comm}
    {
        // log() << "entering TtsLockOwnWindow" << std::endl;
        window.lock_all();
        if (rank == master_rank)
            window.set<uint8_t>(master_rank, locked_disp, 0);
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
        while (window.atomic_get<uint8_t>(master_rank, locked_disp) || window.swap<uint8_t>(master_rank, locked_disp, 1))
            ;
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        window.atomic_set<uint8_t>(master_rank, locked_disp, 0);
        // log() << "exiting release()" << std::endl;
    }
};
