# add_executable(main src/main.cpp)
add_executable(main src/benchmark/main.cpp)
target_include_directories(main PUBLIC src)
target_link_libraries(main bcl_ext)

include_directories($ENV{HOME}/opt/include)

find_library(DASH dash-mpi $ENV{HOME}/opt/lib)
message(STATUS ${DASH})
target_link_libraries(main ${DASH})

find_library(DART_MPI dart-mpi $ENV{HOME}/opt/lib)
message(STATUS ${DART_MPI})
target_link_libraries(main ${DART_MPI})

find_library(DART_BASE dart-base $ENV{HOME}/opt/lib)
message(STATUS ${DART_BASE})
target_link_libraries(main ${DART_BASE})

target_link_libraries(main benchmark::benchmark)
