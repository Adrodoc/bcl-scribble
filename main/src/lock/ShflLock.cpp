#pragma once

#include <mpi.h>
#include "Lock.cpp"
#include "log.cpp"
#include "mpi_utils/mpi_utils.cpp"
#include "mpi_utils/Window.cpp"

class ShflLock : public Lock
{
private:
    static constexpr uint8_t S_WAITING = 0; // Waiting on the node status
    static constexpr uint8_t S_READY = 1;   // The waiter is at the head of the queue
    static constexpr int MAX_SHUFFLES = 1024;

    static constexpr MPI_Aint PADDING = 64;
    enum
    {
        // qnode
        status_disp = 0 * PADDING,      // uint8_t
        socket_disp = 1 * PADDING,      // int
        batch_disp = 2 * PADDING,       // int
        is_shuffler_disp = 3 * PADDING, // bool
        skt_disp = 4 * PADDING,         // int
        next_disp = 5 * PADDING,        // int

        // lock
        glock_disp = 6 * PADDING,          // uint16_t
        no_stealing_disp = glock_disp + 1, // uint8_t
        tail_disp = 7 * PADDING,           // int
    };
    const int master_rank;
    const int rank;
    const int node_id;
    const Window window;

public:
    ShflLock(const ShflLock &) = delete;
    ShflLock(const MPI_Comm comm = MPI_COMM_WORLD, const int master_rank = 0)
        : master_rank{master_rank},
          rank{get_rank(comm)},
          node_id{get_node_id(comm)},
          window{(MPI_Aint)((rank == master_rank ? 8 : 6) * PADDING), comm}
    {
        window.lock_all();
        if (rank == master_rank)
        {
            window.set<uint16_t>(rank, glock_disp, 0);
            window.set(rank, tail_disp, -1);
        }
        window.flush_all();
        MPI_Barrier(comm);
    }

    ~ShflLock()
    {
        window.unlock_all();
    }

    void acquire()
    {
        // Try to steal/acquire the lock if there is no lock holder
        if (window.atomic_get<uint16_t>(master_rank, glock_disp) == 0 &&
            window.compare_and_swap<uint16_t>(master_rank, glock_disp, 0, 1))
            return;

        // Did not get the node, time to join the queue; initialize node states
        window.set(rank, status_disp, S_WAITING);
        window.set(rank, batch_disp, 0);
        window.set(rank, is_shuffler_disp, false);
        window.set(rank, next_disp, -1);
        window.set(rank, skt_disp, node_id);

        int qprev = window.swap(master_rank, tail_disp, rank); // Atomically adding to the queue tail

        if (qprev != -1) // There are waiters ahead
        {
            spin_until_very_next_waiter(qprev);
        }
        else // Disable stealing to maintain the FIFO property
        {
            window.atomic_set<uint8_t>(master_rank, no_stealing_disp, 1); // no_stealing is the second byte of glock
        }

        // qnode is at the head of the queue; time to get the TAS lock
        while (true)
        {
            // Only the very first qnode of the queue becomes the shuffler (line 16)
            // or the one whose socket ID is different from the predecessor
            if (window.get<int>(rank, batch_disp) == 0 || window.get<bool>(rank, is_shuffler_disp))
                shuffle_waiters(true);
            // Wait until the lock holder exits the critical section
            while (window.atomic_get<uint8_t>(master_rank, glock_disp) == 1)
                continue;
            // Try to atomically get the lock
            if (window.compare_and_swap<uint8_t>(master_rank, glock_disp, 0, 1))
                break;
        }

        // MCS unlock phase is moved here
        auto qnext = window.atomic_get<int>(rank, next_disp);
        if (qnext == -1) // qnode is the last one / next pointer is being updated
        {
            // Last one in the queue, reset the tail
            if (window.compare_and_swap(master_rank, tail_disp, rank, -1))
            {
                // Try resetting, else someone joined
                window.compare_and_swap<uint8_t>(master_rank, no_stealing_disp, 1, 0);
                return;
            }
            while ((qnext = window.atomic_get<int>(rank, next_disp)) == -1)
                continue;
        }
        // Notify the very next waiter
        window.atomic_set(qnext, status_disp, S_READY);
    }

    void spin_until_very_next_waiter(int qprev)
    {
        window.atomic_set(qprev, next_disp, rank);
        while (true)
        {
            if (window.atomic_get<uint8_t>(rank, status_disp) == S_READY) // Be ready to hold the lock
                return;
            if (window.atomic_get<bool>(rank, is_shuffler_disp)) // One of the previous shufflers assigned me as a shuffler
                shuffle_waiters(false);
        }
    }

    // A shuffler traverses the queue of waiters (single threaded)
    // and shuffles the queue by bringing the same socket qnodes together
    void shuffle_waiters(bool vnext_waiter)
    {
        int qlast = rank; // Keeps track of shuffled nodes
        // Used for queue traversal
        int qprev = rank;
        int qcurr = -1;
        int qnext = -1;

        // batch → batching within a socket
        int batch = window.get<int>(rank, batch_disp);
        if (batch == 0)
            window.set(rank, batch_disp, ++batch);

        // Shuffler is decided at the end, so clear the value
        window.set(rank, is_shuffler_disp, false);
        // No more batching to avoid starvation
        if (batch >= MAX_SHUFFLES)
            return;

        while (true) // Walking the linked list in sequence
        {
            qcurr = window.get<int>(qprev, next_disp);
            if (qcurr == -1)
                break;
            if (qcurr == window.atomic_get<int>(master_rank, tail_disp)) // Do not shuffle if at the end
                break;

            // NUMA-awareness policy: Group by socket ID
            if (window.get<int>(qcurr, skt_disp) == window.get<int>(rank, skt_disp)) // Found one waiting on the same socket
            {
                if (window.get<int>(qprev, skt_disp) == window.get<int>(rank, skt_disp)) // No shuffling required
                {
                    window.set(qcurr, batch_disp, ++batch);
                    qlast = qprev = qcurr;
                }
                else // Other socket waiters exist between qcurr and qlast
                {
                    qnext = window.get<int>(qcurr, next_disp);
                    if (qnext == -1)
                        break;
                    // Move qcurr after qlast and point qprev.next to qnext
                    window.set(qcurr, batch_disp, ++batch);
                    window.set(qprev, next_disp, qnext);
                    window.set(qcurr, next_disp, window.get<int>(qlast, next_disp));
                    window.set(qlast, next_disp, qcurr);
                    qlast = qcurr; // Update qlast to point to qcurr now
                }
            }
            else // Move on to the next qnode
                qprev = qcurr;

            // Exit → 1) If the very next waiter can acquire the lock
            // 2) A waiter is at the head of the waiting queue
            if ((vnext_waiter == true && window.atomic_get<uint8_t>(master_rank, glock_disp) == 0) ||
                (vnext_waiter == false && window.atomic_get<uint8_t>(rank, status_disp) == S_READY))
                break;
        }
        window.atomic_set(qlast, is_shuffler_disp, true);
    }

    void release()
    {
        window.atomic_set(master_rank, glock_disp, 0);
    }
};
