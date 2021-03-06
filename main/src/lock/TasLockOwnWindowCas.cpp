#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/mpi_utils.cpp"
#include "mpi_utils/Window.cpp"

class TasLockOwnWindowCas : public Lock
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
    TasLockOwnWindowCas(const TasLockOwnWindowCas &) = delete;
    TasLockOwnWindowCas(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)((rank == master_rank ? sizeof(bool) : 0)), comm}
    {
        // log() << "entering TasLockOwnWindowCas" << std::endl;
        window.lock_all();
        if (rank == master_rank)
            window.set(master_rank, locked_disp, false);
        window.flush_all();
        MPI_Barrier(comm);
        // log() << "exiting TasLockOwnWindowCas" << std::endl;
    }

    ~TasLockOwnWindowCas()
    {
        // log() << "entering ~TasLockOwnWindowCas" << std::endl;
        window.unlock_all();
        // log() << "exiting ~TasLockOwnWindowCas" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        while (!window.compare_and_swap(master_rank, locked_disp, false, true))
            ;
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        window.atomic_set(master_rank, locked_disp, false);
        // log() << "exiting release()" << std::endl;
    }
};
