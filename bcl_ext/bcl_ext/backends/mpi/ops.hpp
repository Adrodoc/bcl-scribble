#pragma once

#include <bcl/backends/mpi/ops.hpp>

namespace BCL
{
    // Define datatypes
    template <typename T>
    struct abstract_gptr : public virtual abstract_op<GlobalPtr<T>>
    {
        MPI_Datatype type() const
        {
            return MPI_UINT64_T;
        }
    };

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

    template <typename T>
    struct replace<GlobalPtr<T>> : public abstract_replace<GlobalPtr<T>>, public abstract_gptr<T>, public atomic_op<GlobalPtr<T>>
    {
    };
} // namespace BCL