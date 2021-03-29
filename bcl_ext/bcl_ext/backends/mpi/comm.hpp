#pragma once

#include <bcl/backends/mpi/comm.hpp>
#include "ops.hpp"

namespace BCL
{
    template <typename T>
    inline void op(const GlobalPtr<T> &ptr, const T &val, const atomic_op<T> &op)
    {
        MPI_Request request;
        int error_code = MPI_Raccumulate(&val, 1, op.type(),
                                         ptr.rank, ptr.ptr, 1, op.type(),
                                         op.op(), BCL::win, &request);
        BCL_DEBUG(
            if (error_code != MPI_SUCCESS) {
                throw debug_error("BCL fetch_and_op(): MPI_Rget_accumulate return error code " + std::to_string(error_code));
            })
        error_code = MPI_Wait(&request, MPI_STATUS_IGNORE);
        BCL_DEBUG(
            if (error_code != MPI_SUCCESS) {
                throw debug_error("BCL fetch_and_op(): MPI_Rget_accumulate (MPI_Wait) return error code " + std::to_string(error_code));
            })
    }

    template <typename T>
    inline void atomic_rput(const T &src, const GlobalPtr<T> &dst)
    {
        op(dst, src, replace<T>());
    }

    template <typename T>
    inline T atomic_rget(const GlobalPtr<T> &dst)
    {
        T src;
        return fetch_and_op(dst, src, no_op<T>());
    }

    template <typename T>
    inline GlobalPtr<T> null()
    {
        return nullptr;
    }

    template <typename F, typename S>
    inline GlobalPtr<F> struct_field(GlobalPtr<S> gptr, uint32_t offset)
    {
        return {gptr.rank, gptr.ptr + offset};
    }
} // namespace BCL
