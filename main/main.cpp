#include <bcl/bcl.hpp>
#include <iostream>
#include <SpinLock.cpp>

int main(int argc, char *argv[])
{
  BCL::init();

  BCL::GlobalPtr<int> ptr;
  if (BCL::rank() == 0)
  {
    ptr = BCL::alloc<int>(1);
    *ptr = 0;
  }
  ptr = BCL::broadcast(ptr, 0);
  SpinLock lock{ptr};

  lock.acquire();
  int rank = BCL::rank();
  int nprocs = BCL::nprocs();
  std::cout << "(" << rank << "/" << nprocs << "): lock acquired" << std::endl;
  sleep(1);
  std::cout << "(" << rank << "/" << nprocs << "): releasing lock" << std::endl;
  lock.release();

  BCL::finalize();
  return 0;
}
