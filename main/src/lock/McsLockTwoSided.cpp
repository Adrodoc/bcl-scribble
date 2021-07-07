#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/mpi_utils.cpp"
#include "mpi_utils/Window.cpp"

class McsLockTwoSided : public Lock
{
private:
    static constexpr MPI_Aint PADDING = 64;
    enum
    {
        next_disp = 0 * PADDING,
        tail_disp = 1 * PADDING,
    };
    const MPI_Comm comm;
    const int master_rank;
    const int rank;
    const Window window;
    int *lmem;

public:
    McsLockTwoSided(const McsLockTwoSided &) = delete;
    McsLockTwoSided(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : comm{comm},
          master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)((rank == master_rank ? 2 : 1) * PADDING), comm}
    {
        // log() << "entering McsLockTwoSided" << std::endl;
        int flag;
        MPI_Win_get_attr(window.win, MPI_WIN_BASE, &lmem, &flag);
        window.lock_all();
        if (rank == master_rank)
            window.set(rank, tail_disp, -1);
        window.flush_all();
        MPI_Barrier(comm);
        // log() << "exiting McsLockTwoSided" << std::endl;
    }

    ~McsLockTwoSided()
    {
        // log() << "entering ~McsLockTwoSided" << std::endl;
        window.unlock_all();
        // log() << "exiting ~McsLockTwoSided" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        lmem[next_disp / sizeof(int)] = -1;
        MPI_Win_sync(window.win);

        // log() << "finding predecessor" << std::endl;
        int predecessor = window.swap(master_rank, tail_disp, rank);
        if (predecessor != -1)
        {
            // log() << "notifying predecessor: " << predecessor << std::endl;
            window.atomic_set(predecessor, next_disp, rank);
            MPI_Win_flush(predecessor, window.win);

            // log() << "waiting for predecessor" << std::endl;
            MPI_Recv(0, 0, MPI_INT8_T, predecessor, 0, comm, MPI_STATUS_IGNORE);
            // while (lmem[locked_disp / sizeof(int)])
            //     window.flush_all();
        }
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        int successor = lmem[next_disp / sizeof(int)];
        if (successor == -1)
        {
            // log() << "nulling tail" << std::endl;
            if (window.compare_and_swap(master_rank, tail_disp, rank, -1))
            {
                // log() << "exiting release()" << std::endl;
                return;
            }
            // log() << "waiting for successor" << std::endl;
            while ((successor = lmem[next_disp / sizeof(int)]) == -1)
                window.flush_all();
        }
        // log() << "notifying successor: " << successor << std::endl;
        // window.atomic_set(successor, locked_disp, 0);
        MPI_Ssend(0, 0, MPI_UINT8_T, successor, 0, comm);
        // log() << "exiting release()" << std::endl;
    }
};
