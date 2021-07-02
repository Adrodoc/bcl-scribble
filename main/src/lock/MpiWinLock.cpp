#pragma once

#include <mpi.h>
#include "Lock.cpp"

class MpiWinLock : public Lock
{
private:
    enum
    {
        locked_disp = 0, // bool
    };
    const int master_rank;
    MPI_Win win;

public:
    MpiWinLock(const MpiWinLock &) = delete;
    MpiWinLock(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank}
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        MPI_Aint size = 0;
        if (rank == master_rank)
            size++;
        MPI_Info info;
        MPI_Info_create(&info);
        MPI_Info_set(info, "same_disp_unit", "true");
        uint8_t *mem;
        MPI_Win_allocate(size, sizeof(uint8_t), info, comm, &mem, &win);
        MPI_Barrier(comm);
    }

    ~MpiWinLock()
    {
        // log() << "entering ~MpiWinLock" << std::endl;
        MPI_Win_free(&win);
        // log() << "exiting ~MpiWinLock" << std::endl;
    }

    void acquire()
    {
        // log() << "entering acquire()" << std::endl;
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, master_rank, 0, win);
        uint8_t dummy;
        MPI_Get(&dummy, 1, MPI_UINT8_T, master_rank, 0, 1, MPI_UINT8_T, win);
        MPI_Win_flush(master_rank, win);
        // log() << "exiting acquire()" << std::endl;
    }

    void release()
    {
        // log() << "entering release()" << std::endl;
        MPI_Win_unlock(master_rank, win);
        // log() << "exiting release()" << std::endl;
    }
};
