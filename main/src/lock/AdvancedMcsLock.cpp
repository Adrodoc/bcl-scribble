#include <mpi.h>
#include "Lock.cpp"

class AdvancedMcsLock : public Lock
{
private:
    enum
    {
        nextRank = 0,
        blocked = 1,
        lockTail = 2
    };
    MPI_Comm comm;
    int rank;
    MPI_Win win;
    int *lmem;

public:
    AdvancedMcsLock(const AdvancedMcsLock &) = delete;
    AdvancedMcsLock(MPI_Comm comm = MPI_COMM_WORLD) : comm{comm}
    {
        MPI_Comm_rank(comm, &rank);
        MPI_Aint winsize = 2 * sizeof(int);
        if (rank == 0)
            winsize += sizeof(int);
        MPI_Win_allocate(winsize, sizeof(int), MPI_INFO_NULL, comm, &lmem, &win);
        if (rank == 0)
            lmem[lockTail] = -1;
        MPI_Win_lock_all(0, win);
        MPI_Barrier(comm);
    }

    ~AdvancedMcsLock()
    {
        MPI_Barrier(comm);
        MPI_Win_unlock_all(win);
        MPI_Win_free(&win);
    }

    void acquire()
    {
        lmem[nextRank] = -1;
        lmem[blocked] = 1;
        int predecessor;
        MPI_Fetch_and_op(&rank, &predecessor, MPI_INT,
                         0, lockTail, MPI_REPLACE, win);
        MPI_Win_flush(0, win);
        if (predecessor != -1)
        {
            // We didn’t get the lock. Add us to the tail of the list
            MPI_Accumulate(&rank, 1, MPI_INT,
                           predecessor, nextRank, 1, MPI_INT,
                           MPI_REPLACE, win);
            // Now spin on our local value "blocked" until we are given the lock
            do
            {
                MPI_Win_sync(win); // Ensure memory updated
            } while (lmem[blocked] == 1);
        }
        // else we have the lock
    }

    void release()
    {
        if (lmem[nextRank] == -1)
        {
            // See if we’re waiting for the next to notify us
            int nullrank = -1;
            int curtail;
            MPI_Compare_and_swap(&nullrank, &rank, &curtail, MPI_INT,
                                 0, lockTail, win);
            if (curtail == rank)
            {
                // We are the only process in the list
                return;
            }
            // Otherwise, someone else has added themselves to the list.
            do
            {
                MPI_Win_sync(win);
            } while (lmem[nextRank] == -1);
        }
        // Now we can notify them. Use accumulate with replace instead of put since we want an
        // atomic update of the location
        int zero = 0;
        MPI_Accumulate(&zero, 1, MPI_INT,
                       lmem[nextRank], blocked, 1, MPI_INT,
                       MPI_REPLACE, win);
    }
};
