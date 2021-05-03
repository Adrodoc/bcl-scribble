# Setup

## Install prerequisites

### On Ubuntu
```bash
sudo apt install cmake
# MPICH is recommended, but other MPI implementations might work too. Open-MPI does not work properly at the time of this writing.
sudo apt install mpich
sudo apt install python3-venv
```

## Setup project
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
```

# Build
```bash
cmake --build build --config Release
```

# Run
## Locally
### All benchmarks
```bash
./locally-run-benchmarks.sh
```

### A specific configuration
```bash
export OMPI_MCA_osc=pt2pt # Only needed when using Open-MPI, see https://github.com/open-mpi/ompi/issues/2080
mpirun -n 4 build/main/main
```

## At LRZ
### All benchmarks
```bash
./sbatch-run-benchmarks.sh
```

### A specific configuration
```bash
salloc --ntasks=4 --partition=cm2_inter
mpiexec build/main/main
exit
```
