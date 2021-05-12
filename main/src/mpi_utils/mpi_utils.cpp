#pragma once

#include <mpi.h>

int get_rank(const MPI_Comm comm = MPI_COMM_WORLD)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    return rank;
}
