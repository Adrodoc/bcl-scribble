#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"

class McsLockMpiRequest : public Lock
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
    McsLockMpiRequest(const McsLockMpiRequest &) = delete;
    McsLockMpiRequest(const uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            tail = BCL::alloc<BCL::GlobalPtr<mcs_node>>(1);
            *tail = nullptr;
        }
        tail = BCL::broadcast(tail, rank);

        my_node = BCL::alloc<mcs_node>(1);
    }

    ~McsLockMpiRequest()
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
        MPI_Request request;
        // auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        // BCL::atomic_rput(BCL::null<mcs_node>(), my_node_next);
        auto null = BCL::null<mcs_node>();
        MPI_Raccumulate(&null, 1, MPI_UINT64_T,
                        my_node.rank, my_node.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                        MPI_REPLACE, BCL::win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        // auto predecessor = BCL::fetch_and_op(tail, my_node, BCL::replace<BCL::GlobalPtr<mcs_node>>());
        BCL::GlobalPtr<mcs_node> predecessor;
        MPI_Rget_accumulate(&my_node, 1, MPI_UINT64_T,
                            &predecessor, 1, MPI_UINT64_T,
                            tail.rank, tail.ptr, 1, MPI_UINT64_T,
                            MPI_REPLACE, BCL::win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);

        bool first = predecessor == nullptr;
        if (!first)
        {
            // auto my_node_locked = BCL::struct_field<bool>(my_node, offsetof(mcs_node, locked));
            // BCL::atomic_rput(true, my_node_locked);
            bool b = true;
            MPI_Raccumulate(&b, 1, MPI_CXX_BOOL,
                            my_node.rank, my_node.ptr + offsetof(mcs_node, locked), 1, MPI_CXX_BOOL,
                            MPI_REPLACE, BCL::win, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            // auto predecessor_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(predecessor, offsetof(mcs_node, next));
            // BCL::atomic_rput(my_node, predecessor_next);
            MPI_Raccumulate(&my_node, 1, MPI_UINT64_T,
                            predecessor.rank, predecessor.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                            MPI_REPLACE, BCL::win, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            // while (BCL::atomic_rget(my_node_locked))
            //     ;
            bool locked;
            do
            {
                MPI_Rget_accumulate(0, 1, MPI_CXX_BOOL,
                                    &locked, 1, MPI_CXX_BOOL,
                                    my_node.rank, my_node.ptr + offsetof(mcs_node, locked), 1, MPI_CXX_BOOL,
                                    MPI_NO_OP, BCL::win, &request);
                MPI_Wait(&request, MPI_STATUS_IGNORE);
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
        MPI_Request request;
        // auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        // auto successor = BCL::atomic_rget(my_node_next);
        BCL::GlobalPtr<mcs_node> successor;
        MPI_Rget_accumulate(0, 1, MPI_UINT64_T,
                            &successor, 1, MPI_UINT64_T,
                            my_node.rank, my_node.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                            MPI_NO_OP, BCL::win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        if (successor == nullptr)
        {
            auto null = BCL::null<mcs_node>();
            // auto cas = BCL::compare_and_swap(tail, my_node, null);
            BCL::GlobalPtr<mcs_node> cas;
            MPI_Compare_and_swap(&null, &my_node, &cas, MPI_UINT64_T,
                                 tail.rank, tail.ptr, BCL::win);
            MPI_Win_flush_local(tail.rank, BCL::win);
            if (cas == my_node)
            {
                return true;
            }
        }
        while (successor == nullptr)
        {
            // successor = BCL::atomic_rget(my_node_next);
            MPI_Rget_accumulate(0, 1, MPI_UINT64_T,
                                &successor, 1, MPI_UINT64_T,
                                my_node.rank, my_node.ptr + offsetof(mcs_node, next), 1, MPI_UINT64_T,
                                MPI_NO_OP, BCL::win, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
        // auto successor_locked = BCL::struct_field<bool>(successor, offsetof(mcs_node, locked));
        // BCL::atomic_rput(false, successor_locked);
        bool b = false;
        MPI_Raccumulate(&b, 1, MPI_CXX_BOOL,
                        successor.rank, successor.ptr + offsetof(mcs_node, locked), 1, MPI_CXX_BOOL,
                        MPI_REPLACE, BCL::win, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        return false;
    }
};
