#pragma once

#include <bcl/backends/mpi/comm.hpp>
#include "ops.hpp"

namespace BCL
{
    template <typename T>
    inline void atomic_rput(const T &src, const GlobalPtr<T> &dst)
    {
        fetch_and_op(dst, src, replace<T>());
    }

    template <typename T>
    inline T atomic_rget(const GlobalPtr<T> &dst)
    {
        T src;
        return fetch_and_op(dst, src, no_op<T>());
    }
} // namespace BCL
