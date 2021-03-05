#include <bcl/bcl.hpp>
#include "McsLock.cpp"
#include "log.cpp"

int main(int argc, char *argv[])
{
  BCL::init();

  MPI_Comm local_comm;
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &local_comm);
  int local_rank;
  MPI_Comm_rank(local_comm, &local_rank);
  log() << "local_rank=" << local_rank << std::endl;

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
