#include <mgr/manager.h>
#include <config.h>
#include <chrono>
#include <iostream>

using namespace mgr;

void Manager::manager_main() {
    while (!should_stop.load(std::memory_order::relaxed)) {
        const auto now = std::chrono::steady_clock::now();
        const auto wake_time = now + config::MGR_TICK_DURATION;

        // TODO: only add more jobs if the queue is not too full?
        // FIXME: in debug mode chunk loading can be slow, sometimes causing them to not load at all

        // Load chunks around the player
        auto shared_state = _shared_state.get();
        _chunk_store.load_n_around_on_pool(thread_pool, shared_state.chunk_x, shared_state.chunk_y,
                                           shared_state.chunk_z, config::RENDER_DISTANCE + 2);

        std::this_thread::sleep_until(wake_time);
    }
}

Manager::Manager(SharedStateView initial_state, uint32_t worldgen_seed)
    : _chunk_store(config::MAX_CHUNKS_LOADED, worldgen_seed),
      _shared_state(initial_state),
      thread_pool(config::mgr_thread_count()) {
    manager_thread = std::thread(&Manager::manager_main, this);
}

Manager::~Manager() {
    should_stop.store(true, std::memory_order::relaxed);

    if (manager_thread.joinable()) manager_thread.join();
}
