#pragma once

#include <chrono>
#include <thread>

namespace config {
// Should be even...
constexpr unsigned int BLOCK_SIZE = 80;

constexpr std::chrono::duration<float> MGR_TICK_DURATION = std::chrono::milliseconds(1000 / 20);

inline size_t mgr_thread_count() { return std::max(1u, std::thread::hardware_concurrency() - 1); }

// The maximum chunk indices that can exist

// Must be between -2^23 and 2^23 - 1
constexpr int MIN_CHUNK_X = -8000000;
constexpr int MAX_CHUNK_X = 8000000 - 1;
constexpr int MIN_CHUNK_Z = -8000000;
constexpr int MAX_CHUNK_Z = 8000000 - 1;

// Must be between -2^15 and 2^15 - 1
constexpr int MIN_CHUNK_Y = -3;
constexpr int MAX_CHUNK_Y = 2;

// Render distance in chunks
// Should not be more than a few thousand ish...
constexpr int RENDER_DISTANCE = 10;

// The maximum number of chunks that can be loaded at once
constexpr size_t MAX_CHUNKS_LOADED =
    2 * (2 * (RENDER_DISTANCE + 2) + 1) * (2 * (RENDER_DISTANCE + 2) + 1) * (2 * (RENDER_DISTANCE + 2) + 1);
}  // namespace config