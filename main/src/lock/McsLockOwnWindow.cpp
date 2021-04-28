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

#define REQUEST_BASED

    inline int get(int target_rank, MPI_Aint target_disp)
    {
        int result;
#ifndef REQUEST_BASED
        MPI_Get(&result, 1, MPI_INT,
                target_rank, target_disp, 1, MPI_INT,
                win);
        MPI_Win_flush_local(target_rank, win);
#else
        MPI_Request request;
        MPI_Rget(&result, 1, MPI_INT,
                 target_rank, target_disp, 1, MPI_INT,
                 win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
#endif
        return result;
    }

    inline void set(int target_rank, MPI_Aint target_disp, int value)
    {
#ifndef REQUEST_BASED
        MPI_Put(&value, 1, MPI_INT,
                target_rank, target_disp, 1, MPI_INT,
                win);
        MPI_Win_flush_local(target_rank, win);
#else
        MPI_Request request;
        MPI_Rput(&value, 1, MPI_INT,
                 target_rank, target_disp, 1, MPI_INT,
                 win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
#endif
    }

    inline int atomic_get(int target_rank, MPI_Aint target_disp)
    {
        int dummy;
        int result;
#ifndef REQUEST_BASED
        MPI_Fetch_and_op(&dummy, &result, MPI_INT,
                         target_rank, target_disp, MPI_NO_OP, win);
        // MPI_Get_accumulate(&dummy, 1, MPI_INT,
        //                    &result, 1, MPI_INT,
        //                    target_rank, target_disp, 1, MPI_INT,
        //                    MPI_NO_OP, win);
        MPI_Win_flush_local(target_rank, win);
#else
        MPI_Request request;
        MPI_Rget_accumulate(&dummy, 1, MPI_INT,
                            &result, 1, MPI_INT,
                            target_rank, target_disp, 1, MPI_INT,
                            MPI_NO_OP, win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
#endif
        return result;
    }

    inline void atomic_set(int target_rank, MPI_Aint target_disp, int value)
    {
#ifndef REQUEST_BASED
        // MPI_Accumulate(&value, 1, MPI_INT,
        //                target_rank, target_disp, 1, MPI_INT,
        //                MPI_REPLACE, win);
        // MPI_Win_flush_local(target_rank, win);
#else
        MPI_Request request;
        MPI_Raccumulate(&value, 1, MPI_INT,
                        target_rank, target_disp, 1, MPI_INT,
                        MPI_REPLACE, win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
#endif
    }

    inline int swap(int target_rank, MPI_Aint target_disp, int value)
    {
        int result;
#ifndef REQUEST_BASED
        MPI_Fetch_and_op(&value, &result, MPI_INT,
                         target_rank, target_disp, MPI_REPLACE, win);
        // MPI_Get_accumulate(&value, 1, MPI_INT,
        //                    &result, 1, MPI_INT,
        //                    target_rank, target_disp, 1, MPI_INT,
        //                    MPI_REPLACE, win);
        MPI_Win_flush_local(target_rank, win);
#else
        MPI_Request request;
        MPI_Rget_accumulate(&value, 1, MPI_INT,
                            &result, 1, MPI_INT,
                            target_rank, target_disp, 1, MPI_INT,
                            MPI_REPLACE, win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
#endif
        return result;
    }

    inline bool compare_and_swap(int target_rank, MPI_Aint target_disp, int old_value, int new_value)
    {
        int result;
        MPI_Compare_and_swap(&new_value, &old_value, &result, MPI_INT,
                             target_rank, target_disp, win);
        MPI_Win_flush_local(target_rank, win);
        return result == old_value;
    }

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

        set(rank, locked_disp, 1);
        set(rank, next_disp, -1);

        // log() << "finding predecessor" << std::endl;
        int predecessor = swap(master_rank, tail_disp, rank);
        MPI_Win_flush_all(win);
        if (predecessor != -1)
        {
            // log() << "notifying predecessor: " << predecessor << std::endl;
            atomic_set(predecessor, next_disp, rank);
            // log() << "waiting for predecessor" << std::endl;
            while (atomic_get(rank, locked_disp))
                MPI_Win_flush_all(win);
            ;
        }
        // MPI_Win_unlock_all(win);
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        // MPI_Win_lock_all(0, win);
        int successor = atomic_get(rank, next_disp);
        if (successor == -1)
        {
            // log() << "nulling tail" << std::endl;
            bool b = compare_and_swap(master_rank, tail_disp, rank, -1);
            MPI_Win_flush_all(win);
            if (b)
            {
                // MPI_Win_unlock_all(win);
                // log() << "exiting release()" << std::endl;
                return;
            }
            // log() << "waiting for successor" << std::endl;
            do
            {
                MPI_Win_flush_all(win);
                successor = atomic_get(rank, next_disp);
            } while (successor == -1);
        }
        // log() << "notifying successor: " << successor << std::endl;
        atomic_set(successor, locked_disp, 0);
        // MPI_Win_unlock_all(win);
        // log() << "exiting release()" << std::endl;
    }
};
