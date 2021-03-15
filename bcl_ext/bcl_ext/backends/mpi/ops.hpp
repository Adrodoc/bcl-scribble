#pragma once

#include <bcl/backends/mpi/ops.hpp>

namespace BCL
{
    // Define datatypes
    struct abstract_bool : public virtual abstract_op<bool>
    {
        MPI_Datatype type() const { return MPI_CXX_BOOL; }
    };

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
    struct replace<bool> : public abstract_replace<bool>, public abstract_bool, public atomic_op<bool>
    {
    };

    template <>
    struct replace<int> : public abstract_replace<int>, public abstract_int, public atomic_op<int>
    {
    };

    template <typename T>
    struct replace<GlobalPtr<T>> : public abstract_replace<GlobalPtr<T>>, public abstract_gptr<T>, public atomic_op<GlobalPtr<T>>
    {
    };

    // Define the no_op operation
    template <typename T>
    struct abstract_no_op : public virtual abstract_op<T>
    {
        MPI_Op op() const { return MPI_NO_OP; }
    };

    template <typename T>
    struct no_op;

    template <>
    struct no_op<bool> : public abstract_no_op<bool>, public abstract_bool, public atomic_op<bool>
    {
    };

    template <>
    struct no_op<int> : public abstract_no_op<int>, public abstract_int, public atomic_op<int>
    {
    };

    template <typename T>
    struct no_op<GlobalPtr<T>> : public abstract_no_op<GlobalPtr<T>>, public abstract_gptr<T>, public atomic_op<GlobalPtr<T>>
    {
    };
} // namespace BCL