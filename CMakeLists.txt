cmake_minimum_required(VERSION 3.16)
include(FetchContent)

project(scribble)



FetchContent_Declare(bcl GIT_REPOSITORY https://github.com/berkeley-container-library/bcl.git)
FetchContent_MakeAvailable(bcl)
add_library(bcl INTERFACE)
target_include_directories(bcl INTERFACE ${bcl_SOURCE_DIR})

find_package(MPI REQUIRED)
message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
target_link_libraries(bcl INTERFACE MPI::MPI_CXX)



add_subdirectory(bcl_ext)



add_subdirectory(main)
