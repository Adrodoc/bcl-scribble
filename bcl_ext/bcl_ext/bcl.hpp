#pragma once

#include <bcl/bcl.hpp>

#ifdef SHMEM
#include <bcl_ext/backends/shmem/backend.hpp>
#elif GASNET_EX
#include <bcl_ext/backends/gasnet-ex/backend.hpp>
#elif UPCXX
#include <bcl_ext/backends/upcxx/backend.hpp>
#else
#include <bcl_ext/backends/mpi/backend.hpp>
#endif
