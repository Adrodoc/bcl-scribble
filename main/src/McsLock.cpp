#include <bcl_ext/bcl.hpp>

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
        BCL::rput(BCL::null<mcs_node>(), my_node_next);

        // my_node.local()->next = BCL::GlobalPtr<mcs_node>(nullptr);
        // mcs_node n = {BCL::GlobalPtr<mcs_node>(nullptr), false};
        // BCL::write(&n, my_node, 1);
    }

    ~McsLock()
    {
        if (tail.is_local())
        {
            BCL::dealloc(tail);
        }
        BCL::dealloc(my_node);
    }

    void acquire()
    {
        auto predesessor = BCL::fetch_and_op(tail, my_node, BCL::replace<BCL::GlobalPtr<mcs_node>>());
        auto predesessor_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(predesessor, offsetof(mcs_node, next));

        if (predesessor != nullptr)
        {
            auto my_node_locked = BCL::struct_field<bool>(my_node, offsetof(mcs_node, locked));
            BCL::rput(true, my_node_locked);
            //     my_node.local()->locked = true;

            BCL::rput(my_node, predesessor_next);
            //     predesessor->next = my_node;

            while (BCL::rget(my_node_locked))
                ;
            //     while (my_node.local()->locked)
            //         ;
        }
    }

    void release()
    {
        auto null = BCL::null<mcs_node>();
        auto my_node_next = BCL::struct_field<BCL::GlobalPtr<mcs_node>>(my_node, offsetof(mcs_node, next));
        auto successor = BCL::rget(my_node_next);
        // auto successor = my_node.local()->next;
        if (successor == null)
        {
            if (BCL::compare_and_swap(tail, my_node, null) == null)
            {
                return;
            }
        }
        do
        {
            auto successor = BCL::rget(my_node_next);
            //     successor = my_node.local()->next;
        } while (successor == null);

        auto successor_locked = BCL::struct_field<bool>(successor, offsetof(mcs_node, locked));
        BCL::rput(true, successor_locked);
        // successor->locked = false;
    }
};
