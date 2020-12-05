cmake_minimum_required(VERSION 3.16)

project(scribble)



add_library(bcl INTERFACE)
target_include_directories(bcl INTERFACE bcl)

find_package(MPI REQUIRED)
message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
target_link_libraries(bcl INTERFACE MPI::MPI_CXX)



add_subdirectory(main)
