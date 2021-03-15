# Setup
```bash
# Check out the repository:
git clone https://github.com/Adrodoc/distributed-locks.git
# Go to the root directory:
cd distributed-locks
# Make a build directory to place the build output:
cmake -E make_directory "build"
# Generate build system files with cmake:
cmake -E chdir "build" cmake -DCMAKE_BUILD_TYPE=Release ../
# or, starting with CMake 3.13, use a simpler form:
# cmake -DCMAKE_BUILD_TYPE=Release -S . -B "build"
# Build the project:
cmake --build build --config Release
```

# Run
## Locally
```bash
export OMPI_MCA_osc=pt2pt # Only needed when using Open-MPI, see https://github.com/open-mpi/ompi/issues/2080
mpirun -n 4 build/main/main
```

## At LRZ
```bash
salloc --ntasks=4 --partition=cm2_inter
mpiexec build/main/main
exit
```
