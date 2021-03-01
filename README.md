# Setup
* Clone this repo: `git clone`
* Go into the clone: `cd bcl-scribble`
* Setup cmake: `cmake -S . -B build`

## Compile
`cmake --build build`

## Run
### Locally
```bash
export OMPI_MCA_osc=pt2pt # See https://github.com/open-mpi/ompi/issues/2080
mpirun -n 3 build/main/main
```

### At LRZ
```bash
salloc --ntasks=3 --partition=cm2_inter
mpiexec build/main/main
exit
```
