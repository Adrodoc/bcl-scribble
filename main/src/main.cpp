#include <bcl/bcl.hpp>
#include "McsLock.cpp"
#include "log.cpp"

int main(int argc, char *argv[])
{
  BCL::init();
  int rank = BCL::rank();
  int nprocs = BCL::nprocs();

  MPI_Comm local_comm; // Communicator for inside local node
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &local_comm);
  int local_rank; // Rank inside local node
  MPI_Comm_rank(local_comm, &local_rank);
  int node_id = rank; // The unique ID of the current node
  MPI_Bcast(&node_id, 1, MPI_INT, 0, local_comm);
  MPI_Comm_free(&local_comm);
  char processor_name_array[MPI_MAX_PROCESSOR_NAME];
  int processor_name_len;
  MPI_Get_processor_name(processor_name_array, &processor_name_len);
  std::string processor_name{processor_name_array, processor_name_len};
  log() << "local_rank=" << local_rank << ", node_id=" << node_id << ", processor_name=" << processor_name << std::endl;

  McsLock lock;

  lock.acquire();
  log() << "lock acquired" << std::endl;
  // sleep(1);
  log() << "releasing lock" << std::endl;
  lock.release();

  BCL::finalize();
  return 0;
}
