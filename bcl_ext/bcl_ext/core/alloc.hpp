#pragma once

#include <bcl/core/alloc.hpp>

namespace BCL
{
    template <typename T>
    inline GlobalPtr<T> alloc_shared(const size_t size, const size_t rank = 0)
    {
        BCL::GlobalPtr<T> ptr;
        if (BCL::rank() == rank)
        {
            ptr = BCL::alloc<T>(size);
            *ptr = T{};
        }
        return BCL::broadcast(ptr, rank);
    }
}