#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/mpi_utils.cpp"
#include "mpi_utils/Window.cpp"

class McsLockOwnWindow : public Lock
{
private:
    // struct memory_layout
    // {
    //     bool locked;
    //     int next;
    //     int tail;
    // };
    // static constexpr MPI_Aint locked_disp = offsetof(memory_layout, locked);
    // static constexpr MPI_Aint next_disp = offsetof(memory_layout, next);
    // static constexpr MPI_Aint tail_disp = offsetof(memory_layout, tail);
    static constexpr MPI_Aint PADDING = 64;
    enum
    {
        locked_disp = 0 * PADDING,
        next_disp = 1 * PADDING,
        tail_disp = 2 * PADDING,
    };
    const int master_rank;
    const int rank;
    const Window window;

public:
    McsLockOwnWindow(const McsLockOwnWindow &) = delete;
    McsLockOwnWindow(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)((rank == master_rank ? 3 : 2) * PADDING), comm}
    {
        // log() << "entering McsLockOwnWindow" << std::endl;
        window.lock_all();
        if (rank == master_rank)
            window.set(rank, tail_disp, -1);
        window.flush_all();
        MPI_Barrier(comm);
        // log() << "exiting McsLockOwnWindow" << std::endl;
    }

    ~McsLockOwnWindow()
    {
        // log() << "entering ~McsLockOwnWindow" << std::endl;
        window.unlock_all();
        // log() << "exiting ~McsLockOwnWindow" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        window.atomic_set(rank, locked_disp, true);
        window.atomic_set(rank, next_disp, -1);

        // log() << "finding predecessor" << std::endl;
        int predecessor = window.swap(master_rank, tail_disp, rank);
        if (predecessor != -1)
        {
            // log() << "notifying predecessor: " << predecessor << std::endl;
            window.atomic_set(predecessor, next_disp, rank);
            // log() << "waiting for predecessor" << std::endl;
            while (window.atomic_get<bool>(rank, locked_disp))
                window.flush_all();
        }
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        int successor = window.atomic_get<int>(rank, next_disp);
        if (successor == -1)
        {
            // log() << "nulling tail" << std::endl;
            if (window.compare_and_swap(master_rank, tail_disp, rank, -1))
            {
                // log() << "exiting release()" << std::endl;
                return;
            }
            // log() << "waiting for successor" << std::endl;
            while ((successor = window.atomic_get<int>(rank, next_disp)) == -1)
                window.flush_all();
        }
        // log() << "notifying successor: " << successor << std::endl;
        window.atomic_set(successor, locked_disp, false);
        // log() << "exiting release()" << std::endl;
    }
};
