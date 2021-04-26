#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"

class AdvancedMcsLock : public Lock
{
private:
    enum
    {
        nextRank = 0,
        blocked = 1,
        lockTail = 2
    };
    int rank;
    MPI_Win win;
    int *lmem;

public:
    AdvancedMcsLock(const AdvancedMcsLock &) = delete;
    AdvancedMcsLock(MPI_Comm comm = MPI_COMM_WORLD)
    {
        MPI_Comm_rank(comm, &rank);
        MPI_Aint winsize = 2 * sizeof(int);
        if (rank == 0)
            winsize += sizeof(int);
        MPI_Win_allocate(winsize, sizeof(int), MPI_INFO_NULL, comm, &lmem, &win);
        if (rank == 0)
            lmem[lockTail] = -1;
        MPI_Win_fence(0, win);
        MPI_Win_lock_all(0, win);
    }

    ~AdvancedMcsLock()
    {
        MPI_Win_unlock_all(win);
        MPI_Win_free(&win);
    }

    void acquire()
    {
        log() << "entering acquire()" << std::endl;
        lmem[nextRank] = -1;
        lmem[blocked] = 1;
        log() << "finding predecessor" << std::endl;
        int predecessor;
        MPI_Fetch_and_op(&rank, &predecessor, MPI_INT,
                         0, lockTail, MPI_REPLACE, win);
        MPI_Win_flush(0, win);
        if (predecessor != -1)
        {
            // We didn’t get the lock. Add us to the tail of the list
            log() << "notifying predecessor: " << predecessor << std::endl;
            MPI_Accumulate(&rank, 1, MPI_INT,
                           predecessor, nextRank, 1, MPI_INT,
                           MPI_REPLACE, win);
            // Now spin on our local value "blocked" until we are given the lock
            log() << "waiting for predecessor" << std::endl;
            do
            {
                MPI_Win_sync(win); // Ensure memory updated
            } while (lmem[blocked] == 1);
        }
        // else we have the lock
        log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        log() << "entering release()" << std::endl;
        if (lmem[nextRank] == -1)
        {
            // See if we’re waiting for the next to notify us
            log() << "nulling tail" << std::endl;
            int nullrank = -1;
            int curtail;
            MPI_Compare_and_swap(&nullrank, &rank, &curtail, MPI_INT,
                                 0, lockTail, win);
            if (curtail == rank)
            {
                // We are the only process in the list
                log() << "exiting release()" << std::endl;
                return;
            }
            // Otherwise, someone else has added themselves to the list.
            log() << "waiting for successor (tail=" << curtail << ")" << std::endl;
            do
            {
                MPI_Win_sync(win);
            } while (lmem[nextRank] == -1);
        }
        // Now we can notify them. Use accumulate with replace instead of put since we want an
        // atomic update of the location
        log() << "notifying successor: " << lmem[nextRank] << std::endl;
        int zero = 0;
        MPI_Accumulate(&zero, 1, MPI_INT,
                       lmem[nextRank], blocked, 1, MPI_INT,
                       MPI_REPLACE, win);
        log() << "exiting release()" << std::endl;
    }
};
