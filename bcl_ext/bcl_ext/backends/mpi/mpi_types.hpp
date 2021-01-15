#pragma once

#include <bcl/bcl.hpp>

namespace BCL
{
  template <typename T>
  struct get_mpi_type_impl_<GlobalPtr<T>>
  {
    static MPI_Datatype mpi_type() { return MPI_INT64_T; }
  };
} // namespace BCL
