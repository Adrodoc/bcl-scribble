#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
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
    enum
    {
        locked_disp = 0,
        next_disp = 128,
        tail_disp = 128 * 2
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
    McsLockOwnWindow(const McsLockOwnWindow &) = delete;
    McsLockOwnWindow(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)(128 * 3), comm}
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
                MPI_Win_flush_all(window.win);
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
            do
            {
                successor = window.atomic_get<int>(rank, next_disp);
                MPI_Win_flush_all(window.win);
            } while (successor == -1);
        }
        // log() << "notifying successor: " << successor << std::endl;
        window.atomic_set(successor, locked_disp, false);
        // log() << "exiting release()" << std::endl;
    }
};
