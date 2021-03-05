#include <bcl/bcl.hpp>
#include "McsLock.cpp"
#include "log.cpp"

int main(int argc, char *argv[])
{
  BCL::init();

  MPI_Comm local_comm; // Communicator for inside local node
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &local_comm);
  int local_rank; // Rank inside local node
  MPI_Comm_rank(local_comm, &local_rank);
  int node_id = local_rank; // The unique ID of the current node
  MPI_Bcast(&node_id, 1, MPI_INT, 0, local_comm);
  MPI_Comm_free(&local_comm);
  log() << "local_rank=" << local_rank << "node_id=" << node_id << std::endl;

  McsLock lock;

  lock.acquire();
  int rank = BCL::rank();
  int nprocs = BCL::nprocs();
  log() << "lock acquired" << std::endl;
  // sleep(1);
  log() << "releasing lock" << std::endl;
  lock.release();

  BCL::finalize();
  return 0;
}
