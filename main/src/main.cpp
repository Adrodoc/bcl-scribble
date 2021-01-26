#include <bcl/bcl.hpp>
#include "McsLock.cpp"
#include "log.cpp"

int main(int argc, char *argv[])
{
  BCL::init();

  McsLock lock;

  lock.acquire();
  int rank = BCL::rank();
  int nprocs = BCL::nprocs();
  log() << "lock acquired" << std::endl;
  sleep(1);
  log() << "releasing lock" << std::endl;
  lock.release();

  BCL::finalize();
  return 0;
}
