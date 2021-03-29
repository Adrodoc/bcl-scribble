#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"

class McsLockMpiFlushLocalAll : public Lock
{
private:
    struct mcs_node
    {
        BCL::GlobalPtr<mcs_node> next;
        bool locked;
    };
    BCL::GlobalPtr<BCL::GlobalPtr<mcs_node>> tail;
    BCL::GlobalPtr<mcs_node> my_node;

public:
    McsLockMpiFlushLocalAll(const McsLockMpiFlushLocalAll &) = delete;
    McsLockMpiFlushLocalAll(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            tail = BCL::alloc<BCL::GlobalPtr<mcs_node>>(1);
            *tail = nullptr;
        }
        tail = BCL::broadcast(tail, rank);

        my_node = BCL::alloc<mcs_node>(1);
    }

    ~McsLockMpiFlushLocalAll()
    {
        BCL::barrier();
        if (tail.is_local())
        {
            BCL::dealloc(tail);
        }
        BCL::dealloc(my_node);
    }

    void acquire()
    {
        acquire_ext();
    }
    // Returns true if we acquired the lock immediately, because were the first one in the queue right away.
    bool acquire_ext()
    {
        // auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        // BCL::atomic_rput(BCL::null<mcs_node>(), my_node_next);
        auto null = BCL::null<mcs_node>();
        MPI_Accumulate(&null, 1, MPI_UINT64_T,
                       my_node.rank, my_node.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                       MPI_REPLACE, BCL::win);
        MPI_Win_flush_local_all(BCL::win);
        // auto predecessor = BCL::fetch_and_op(tail, my_node, BCL::replace<BCL::GlobalPtr<mcs_node>>());
        BCL::GlobalPtr<mcs_node> predecessor;
        MPI_Get_accumulate(&my_node, 1, MPI_UINT64_T,
                           &predecessor, 1, MPI_UINT64_T,
                           tail.rank, tail.ptr, 1, MPI_UINT64_T,
                           MPI_REPLACE, BCL::win);
        MPI_Win_flush_local_all(BCL::win);

        bool first = predecessor == nullptr;
        if (!first)
        {
            // auto my_node_locked = BCL::struct_field<bool>(my_node, offsetof(mcs_node, locked));
            // BCL::atomic_rput(true, my_node_locked);
            bool b = true;
            MPI_Accumulate(&b, 1, MPI_CXX_BOOL,
                           my_node.rank, my_node.ptr + offsetof(mcs_node, locked), 1, MPI_CXX_BOOL,
                           MPI_REPLACE, BCL::win);
            // auto predecessor_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(predecessor, offsetof(mcs_node, next));
            // BCL::atomic_rput(my_node, predecessor_next);
            MPI_Accumulate(&my_node, 1, MPI_UINT64_T,
                           predecessor.rank, predecessor.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                           MPI_REPLACE, BCL::win);
            MPI_Win_flush_local_all(BCL::win);
            // while (BCL::atomic_rget(my_node_locked))
            //     ;
            bool locked;
            do
            {
                MPI_Get_accumulate(0, 1, MPI_CXX_BOOL,
                                   &locked, 1, MPI_CXX_BOOL,
                                   my_node.rank, my_node.ptr + offsetof(mcs_node, locked), 1, MPI_CXX_BOOL,
                                   MPI_NO_OP, BCL::win);
                MPI_Win_flush_local_all(BCL::win);
            } while (locked);
        }
        return first;
    }

    void release()
    {
        release_ext();
    }
    // Returns true if we were the last one in the queue. By now someone might have entered the queue again though.
    bool release_ext()
    {
        // auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        // auto successor = BCL::atomic_rget(my_node_next);
        BCL::GlobalPtr<mcs_node> successor;
        MPI_Get_accumulate(0, 1, MPI_UINT64_T,
                           &successor, 1, MPI_UINT64_T,
                           my_node.rank, my_node.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                           MPI_NO_OP, BCL::win);
        MPI_Win_flush_local_all(BCL::win);
        if (successor == nullptr)
        {
            auto null = BCL::null<mcs_node>();
            // auto cas = BCL::compare_and_swap(tail, my_node, null);
            BCL::GlobalPtr<mcs_node> cas;
            MPI_Compare_and_swap(&null, &my_node, &cas, MPI_UINT64_T,
                                 tail.rank, tail.ptr, BCL::win);
            MPI_Win_flush_local_all(BCL::win);
            if (cas == my_node)
            {
                return true;
            }
        }
        while (successor == nullptr)
        {
            // successor = BCL::atomic_rget(my_node_next);
            MPI_Get_accumulate(0, 1, MPI_UINT64_T,
                               &successor, 1, MPI_UINT64_T,
                               my_node.rank, my_node.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                               MPI_NO_OP, BCL::win);
            MPI_Win_flush_local_all(BCL::win);
        }
        // auto successor_locked = BCL::struct_field<bool>(successor, offsetof(mcs_node, locked));
        // BCL::atomic_rput(false, successor_locked);
        bool b = false;
        MPI_Accumulate(&b, 1, MPI_CXX_BOOL,
                       successor.rank, successor.ptr + offsetof(mcs_node, locked), 1, MPI_CXX_BOOL,
                       MPI_REPLACE, BCL::win);
        MPI_Win_flush_local_all(BCL::win);
        return false;
    }
};
