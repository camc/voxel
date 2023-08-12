#pragma once

#include "threadpool.h"
#include "sharedstate.h"
#include "chunkstore.h"
#include <atomic>

namespace mgr {

// Thread running at a fixed rate which manages various game related tasks, using its own thread pool.
// The destructor will block until the manager thread has stopped.
class Manager {
    // SAFETY: The chunk store will outlive the threads as the destructor of the ThreadPool will block until all threads
    // have stopped. The ThreadPool destructor will be called before the ChunkStore destructor as it is declared after.
    ChunkStore _chunk_store;

    std::thread manager_thread;
    SharedState _shared_state;
    std::atomic<bool> should_stop = false;
    ThreadPool thread_pool;

    Manager operator=(const Manager&) = delete;
    Manager(const Manager&) = delete;

    void manager_main();

public:
    // Starts the manager thread.
    Manager(SharedStateView initial_state, uint32_t worldgen_seed);
    ~Manager();

    SharedState& shared_state() { return _shared_state; }
    ChunkStore& chunk_store() { return _chunk_store; }
};

}  // namespace mgr