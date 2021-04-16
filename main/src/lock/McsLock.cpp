#pragma once

#include <bcl_ext/bcl.hpp>
#include "Lock.cpp"
#include "log.cpp"

class McsLock : public Lock
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
    McsLock(const McsLock &) = delete;
    McsLock(const uint64_t rank = 0)
    {
        log() << "entering McsLock" << std::endl;
        if (BCL::rank() == rank)
        {
            tail = BCL::alloc<BCL::GlobalPtr<mcs_node>>(1);
            *tail = nullptr;
        }
        tail = BCL::broadcast(tail, rank);

        my_node = BCL::alloc<mcs_node>(1);
        // my_node.local()->next = BCL::GlobalPtr<mcs_node>(nullptr);
        // mcs_node n = {BCL::GlobalPtr<mcs_node>(nullptr), false};
        // BCL::write(&n, my_node, 1);

        log() << "tail=" << tail << std::endl;
        log() << "my_node=" << my_node << std::endl;
        log() << "exiting McsLock" << std::endl;
    }

    ~McsLock()
    {
        log() << "entering ~McsLock" << std::endl;
        BCL::barrier();
        if (tail.is_local())
        {
            BCL::dealloc(tail);
        }
        BCL::dealloc(my_node);
        log() << "exiting ~McsLock" << std::endl;
    }

    void acquire()
    {
        acquire_ext();
    }
    // Returns true if we acquired the lock immediately, because were the first one in the queue right away.
    bool acquire_ext()
    {
        auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        BCL::atomic_rput(BCL::null<mcs_node>(), my_node_next);
        log() << "entering acquire()" << std::endl;
        log() << "my_node=" << my_node << std::endl;
        auto predecessor = BCL::fetch_and_op(tail, my_node, BCL::replace<BCL::GlobalPtr<mcs_node>>());
        log() << "predecessor=" << predecessor << std::endl;

        log() << "flushing" << std::endl;
        BCL::flush();

        bool first = predecessor == nullptr;
        if (!first)
        {
            auto my_node_locked = BCL::struct_field<bool>(my_node, offsetof(mcs_node, locked));
            log() << "locking my_node at " << my_node << std::endl;
            BCL::atomic_rput(true, my_node_locked);
            //     my_node.local()->locked = true;

            log() << "flushing" << std::endl;
            BCL::flush();

            auto predecessor_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(predecessor, offsetof(mcs_node, next));
            log() << "notifying predecessor at " << predecessor_next << std::endl;

            BCL::atomic_rput(my_node, predecessor_next);
            //     predecessor->next = my_node;

            log() << "flushing" << std::endl;
            BCL::flush();

            log() << "notified predecessor" << std::endl;

            while (BCL::atomic_rget(my_node_locked))
                BCL::flush();
            // log() << "waiting to acquire lock" << std::endl;
            ;
            //     while (my_node.local()->locked)
            //         ;
        }
        log() << "lock acquired" << std::endl;
        return first;
    }

    void release()
    {
        release_ext();
    }
    // Returns true if we were the last one in the queue. By now someone might have entered the queue again though.
    bool release_ext()
    {
        log() << "entering release()" << std::endl;
        auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        auto successor = BCL::atomic_rget(my_node_next);
        // auto successor = my_node.local()->next;
        log() << "successor=" << successor << std::endl;
        if (successor == nullptr)
        {
            auto null = BCL::null<mcs_node>();
            auto cas = BCL::compare_and_swap(tail, my_node, null);
            log() << "cas(" << tail << ", " << my_node << ", " << null << ") = " << cas << std::endl;

            log() << "flushing" << std::endl;
            BCL::flush();

            if (cas == my_node)
            {
                log() << "no successor" << std::endl;
                log() << "lock released" << std::endl;
                return true;
            }
            log() << "waiting for successor at " << my_node_next << std::endl;
        }
        while (successor == nullptr)
        {
            BCL::flush();
            successor = BCL::atomic_rget(my_node_next);
            //     successor = my_node.local()->next;
            // log() << "waiting for successor at " << my_node_next << std::endl;
        }
        log() << "found successor=" << successor << std::endl;

        auto successor_locked = BCL::struct_field<bool>(successor, offsetof(mcs_node, locked));
        BCL::atomic_rput(false, successor_locked);
        // successor->locked = false;

        log() << "flushing" << std::endl;
        BCL::flush();

        log() << "lock released" << std::endl;
        return false;
    }
};
