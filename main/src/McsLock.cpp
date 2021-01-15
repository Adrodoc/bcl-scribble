#include <bcl_ext/bcl.hpp>

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
        my_node.local()->next = BCL::GlobalPtr<mcs_node>(nullptr);
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
        if (predesessor != nullptr)
        {
            my_node.local()->locked = true;

                        predesessor->next = my_node;
            while (my_node.local()->locked)
                ;
        }
    }
    void release()
    {
        auto null = BCL::GlobalPtr<mcs_node>(nullptr);
        auto successor = my_node.local()->next;
        if (successor == null)
        {
            if (BCL::compare_and_swap(tail, my_node, null) == null)
            {
                return;
            }
        }
        do
        {
            successor = my_node.local()->next;
        } while (successor == null);

        successor->locked = false;
    }
};
