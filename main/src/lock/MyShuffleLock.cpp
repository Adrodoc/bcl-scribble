#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/Window.cpp"

class MyShuffleLock : public Lock
{
private:
    static constexpr char LOCKED = 0;
    static constexpr char WAITING_FOR_LOCK = 1;
    static constexpr char SHUFFLING = 2;
    static constexpr char WAITING_FOR_SHUFFLE = 3;
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

    static int get_rank(const MPI_Comm comm)
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        return rank;
    }

public:
    MyShuffleLock(const MyShuffleLock &) = delete;
    MyShuffleLock(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          window{(MPI_Aint)((rank == master_rank ? 3 : 2) * PADDING), comm}
    {
        // log() << "entering MyShuffleLock" << std::endl;
        window.lock_all();
        if (rank == master_rank)
            window.set(rank, tail_disp, -1);
        window.flush_all();
        MPI_Barrier(comm);
        // log() << "exiting MyShuffleLock" << std::endl;
    }

    ~MyShuffleLock()
    {
        // log() << "entering ~MyShuffleLock" << std::endl;
        window.unlock_all();
        // log() << "exiting ~MyShuffleLock" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        char locked = WAITING_FOR_SHUFFLE;
        window.atomic_set(rank, locked_disp, locked);
        window.atomic_set(rank, next_disp, -1);

        // log() << "finding predecessor" << std::endl;
        int predecessor = window.swap(master_rank, tail_disp, rank);
        if (predecessor != -1)
        {
            // log() << "notifying predecessor: " << predecessor << std::endl;
            window.atomic_set(predecessor, next_disp, rank);

            if (window.atomic_get<char>(predecessor, locked_disp) == LOCKED)
            {
                locked = SHUFFLING;
                window.compare_and_swap(rank, locked_disp, WAITING_FOR_SHUFFLE, locked);
            }

            while (locked == WAITING_FOR_SHUFFLE)
            {
                window.flush_all();
                locked = window.atomic_get<char>(rank, locked_disp);
            }

            while (locked == SHUFFLING)
            {
                // shuffle
                window.flush_all();
                locked = window.atomic_get<char>(rank, locked_disp);
            }

            while (locked == WAITING_FOR_LOCK)
            {
                window.flush_all();
                locked = window.atomic_get<char>(rank, locked_disp);
            }

            auto next = window.atomic_get<int>(rank, next_disp);
            if (next != -1)
                window.compare_and_swap(next, locked_disp, WAITING_FOR_SHUFFLE, SHUFFLING);
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
        window.atomic_set(successor, locked_disp, LOCKED);
        // log() << "exiting release()" << std::endl;
    }
};
