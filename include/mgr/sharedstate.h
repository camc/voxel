#pragma once

#include <mutex>
#include <functional>

namespace mgr {

// A view (copy) of the shared state at a given time.
struct SharedStateView {
    int chunk_x;
    int chunk_y;
    int chunk_z;
};

class SharedState {
    std::mutex mutex;
    SharedStateView state;

public:
    SharedState(SharedStateView initial_state) : state(initial_state) {}

    SharedStateView get() {
        std::scoped_lock<std::mutex> lock(mutex);
        return state;
    }

    void modify(const std::function<void(SharedStateView&)>& f) {
        std::scoped_lock<std::mutex> lock(mutex);
        f(state);
    }
};

}  // namespace mgr