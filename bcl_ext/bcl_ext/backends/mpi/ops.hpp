#pragma once

#include <bcl/backends/mpi/ops.hpp>

namespace BCL
{
    // Define the replace operation
    template <typename T>
    struct abstract_replace : public virtual abstract_op<T>
    {
        MPI_Op op() const { return MPI_REPLACE; }
    };

    template <typename T>
    struct replace;

    template <>
    struct replace<int> : public abstract_replace<int>, public abstract_int, public atomic_op<int>
    {
    };
} // namespace BCL