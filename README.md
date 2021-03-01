# Setup
* Clone this repo: `git clone`
* Go into the clone: `cd bcl-scribble`
* Setup cmake: `cmake -S . -B build`

## Compile
`cmake --build build`

## Run
`OMPI_MCA_osc=pt2pt mpirun -n 4 build/main/main` (see https://github.com/open-mpi/ompi/issues/2080)
