#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"

class McsLockOwnWindow : public Lock
{
private:
    enum
    {
        locked_disp = 0,
        next_disp = 1,
        tail_disp = 2
    };
    const int master_rank;
    int rank;
    int *mem;
    MPI_Win win;

public:
    McsLockOwnWindow(const McsLockOwnWindow &) = delete;
    McsLockOwnWindow(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0) : master_rank{master_rank}
    {
        // log() << "entering McsLockOwnWindow" << std::endl;
        MPI_Comm_rank(comm, &rank);
        MPI_Aint win_size = (rank == master_rank ? 3 : 2) * sizeof(int);
        MPI_Win_allocate(win_size, sizeof(int), MPI_INFO_NULL, comm, &mem, &win);
        if (rank == 0)
            mem[tail_disp] = -1;
        MPI_Win_fence(0, win);
        MPI_Win_lock_all(0, win);
        // log() << "exiting McsLockOwnWindow" << std::endl;
    }

    ~McsLockOwnWindow()
    {
        // log() << "entering ~McsLockOwnWindow" << std::endl;
        MPI_Win_unlock_all(win);
        MPI_Win_free(&win);
        // log() << "exiting ~McsLockOwnWindow" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        // mem[locked_disp] = 1;
        // mem[next_disp] = -1;
        // MPI_Win_lock_all(0, win);
        int one = 1;
        MPI_Put(&one, 1, MPI_INT,
                rank, locked_disp, 1, MPI_INT,
                win);
        int nullrank = -1;
        MPI_Put(&nullrank, 1, MPI_INT,
                rank, next_disp, 1, MPI_INT,
                win);
        MPI_Win_flush(rank, win);

        // log() << "finding predecessor" << std::endl;
        int predecessor;
        MPI_Fetch_and_op(&rank, &predecessor, MPI_INT,
                         master_rank, tail_disp, MPI_REPLACE, win);
        MPI_Win_flush_local(master_rank, win);
        if (predecessor != -1)
        {
            // log() << "notifying predecessor: " << predecessor << std::endl;
            MPI_Accumulate(&rank, 1, MPI_INT,
                           predecessor, next_disp, 1, MPI_INT,
                           MPI_REPLACE, win);
            MPI_Win_flush(predecessor, win);
            // MPI_Win_flush_all(win);
            // log() << "waiting for predecessor" << std::endl;
            int locked;
            do
            {
                MPI_Get_accumulate(0, 0, MPI_INT,
                                   &locked, 1, MPI_INT,
                                   rank, locked_disp, 1, MPI_INT,
                                   MPI_NO_OP, win);
                MPI_Win_flush(rank, win);
            } while (locked);
        }
        // MPI_Win_unlock_all(win);
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        // MPI_Win_lock_all(0, win);
        int successor;
        MPI_Get_accumulate(0, 0, MPI_INT,
                           &successor, 1, MPI_INT,
                           rank, next_disp, 1, MPI_INT,
                           MPI_NO_OP, win);
        MPI_Win_flush_local(rank, win);
        if (successor == -1)
        {
            // log() << "nulling tail" << std::endl;
            int nullrank = -1;
            int tail;
            MPI_Compare_and_swap(&nullrank, &rank, &tail, MPI_INT,
                                 master_rank, tail_disp, win);
            MPI_Win_flush_local(master_rank, win);
            if (tail == rank)
            {
                // MPI_Win_unlock_all(win);
                // log() << "exiting release()" << std::endl;
                return;
            }
            // log() << "waiting for successor (tail=" << tail << ")" << std::endl;
            do
            {
                MPI_Get_accumulate(0, 0, MPI_INT,
                                   &successor, 1, MPI_INT,
                                   rank, next_disp, 1, MPI_INT,
                                   MPI_NO_OP, win);
                MPI_Win_flush(rank, win);
            } while (successor == -1);
        }
        // log() << "notifying successor: " << successor << std::endl;
        int zero = 0;
        MPI_Accumulate(&zero, 1, MPI_INT,
                       successor, locked_disp, 1, MPI_INT,
                       MPI_REPLACE, win);
        MPI_Win_flush_local(successor, win);
        // MPI_Win_unlock_all(win);
        // log() << "exiting release()" << std::endl;
    }
};
