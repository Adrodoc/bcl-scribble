#include <bcl_ext/bcl.hpp>
#include "log.cpp"

namespace BCL
{
    template <typename T>
    GlobalPtr<T> null()
    {
        return nullptr;
    }

    template <typename F, typename S>
    GlobalPtr<F> struct_field(GlobalPtr<S> gptr, uint32_t offset)
    {
        return {gptr.rank, gptr.ptr + offset};
    }
} // namespace BCL

struct mcs_node
{
    BCL::GlobalPtr<mcs_node> next;
    bool locked;
};

class McsLock
{
private:
    BCL::GlobalPtr<BCL::GlobalPtr<mcs_node>> tail;
    BCL::GlobalPtr<mcs_node> my_node;

public:
    McsLock(uint64_t rank = 0)
    {
        if (BCL::rank() == rank)
        {
            tail = BCL::alloc<BCL::GlobalPtr<mcs_node>>(1);
            *tail = nullptr;
        }
        tail = BCL::broadcast(tail, rank);

        my_node = BCL::alloc<mcs_node>(1);
        auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        BCL::atomic_rput(BCL::null<mcs_node>(), my_node_next);
        // my_node.local()->next = BCL::GlobalPtr<mcs_node>(nullptr);
        // mcs_node n = {BCL::GlobalPtr<mcs_node>(nullptr), false};
        // BCL::write(&n, my_node, 1);

        log() << "tail=" << tail << std::endl;
        log() << "my_node=" << my_node << std::endl;
    }

    ~McsLock()
    {
        // TODO dealloc
        // if (tail.is_local())
        // {
        //     BCL::dealloc(tail);
        // }
        // BCL::dealloc(my_node);
    }

    void acquire()
    {
        log() << "entering acquire()" << std::endl;
        log() << "my_node=" << my_node << std::endl;
        auto predecessor = BCL::fetch_and_op(tail, my_node, BCL::replace<BCL::GlobalPtr<mcs_node>>());
        log() << "predecessor=" << predecessor << std::endl;

        if (predecessor != nullptr)
        {
            auto my_node_locked = BCL::struct_field<bool>(my_node, offsetof(mcs_node, locked));
            log() << "locking my_node at " << my_node << std::endl;
            BCL::atomic_rput(true, my_node_locked);
            //     my_node.local()->locked = true;

            auto predecessor_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(predecessor, offsetof(mcs_node, next));
            log() << "notifying predecessor at " << predecessor_next << std::endl;

            // MPI_Accumulate(&my_node, 1, MPI_UINT64_T,
            //                predecessor_next.rank, predecessor_next.ptr, 1, MPI_UINT64_T,
            //                MPI_REPLACE, BCL::win);
            // log() << "after MPI_Accumulate" << std::endl;
            // MPI_Win_flush_local(predecessor_next.rank, BCL::win);
            // log() << "after MPI_Win_local_all" << std::endl;

            // MPI_Request request;
            // MPI_Raccumulate(&my_node, 1, MPI_UINT64_T,
            //                 predecessor_next.rank, predecessor_next.ptr, 1, MPI_UINT64_T,
            //                 MPI_REPLACE, BCL::win, &request);
            // log() << "after MPI_Raccumulate" << std::endl;
            // MPI_Wait(&request, MPI_STATUS_IGNORE);
            // log() << "after MPI_Wait" << std::endl;

            BCL::atomic_rput(my_node, predecessor_next);
            //     predecessor->next = my_node;

            log() << "notified predecessor" << std::endl;

            while (BCL::atomic_rget(my_node_locked))
                // MPI_Win_flush_local_all(BCL::win);
                // MPI_Win_sync(BCL::win);
                ;
            //     while (my_node.local()->locked)
            //         ;
        }
    }

    void release()
    {
        auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        auto successor = BCL::atomic_rget(my_node_next);
        // auto successor = my_node.local()->next;
        log() << "successor=" << successor << std::endl;
        if (successor == nullptr)
        {
            auto null = BCL::null<mcs_node>();
            auto cas = BCL::compare_and_swap(tail, my_node, null);
            log() << "cas(" << tail << ", " << my_node << ", " << null << ") = " << cas << std::endl;
            if (cas == my_node)
            {
                log() << "no successor" << std::endl;
                log() << "lock released" << std::endl;
                return;
            }
        }
        log() << "waiting for successor at " << my_node_next << std::endl;
        while (successor == nullptr)
        {
            successor = BCL::atomic_rget(my_node_next);
            //     successor = my_node.local()->next;
            // log() << "while successor=" << successor << std::endl;
        }
        log() << "found successor=" << successor << std::endl;

        auto successor_locked = BCL::struct_field<bool>(successor, offsetof(mcs_node, locked));
        BCL::atomic_rput(false, successor_locked);
        // successor->locked = false;

        log() << "lock released" << std::endl;
    }
};
