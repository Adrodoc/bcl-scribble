#include <bcl/bcl.hpp>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
  BCL::init();
  int rank = BCL::rank();
  int size = BCL::nprocs();
  cout << "Hello MPI! I am " << rank << " of " << size << endl;
  BCL::finalize();
  return 0;
}
