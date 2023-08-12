#pragma once

#include <list>
#include <unordered_map>
#include <optional>
#include "../models/chunk.h"
#include "../render/chunk.h"
#include "threadpool.h"
#include <functional>
#include <tuple>
#include "../worldgen/generator.h"

namespace mgr {

struct ChunkStoreEntry {
    models::RenderingChunk chunk;
    std::vector<uint8_t> vertex_data;
    unsigned int instance_count;
};

// One thread should have access to this at a time.
class ChunkStoreHandle {
    using ChunkCoord = std::tuple<int, int, int>;

    struct ChunkCoordHasher {
        std::size_t operator()(const ChunkCoord& coord) const {
            const auto [chunk_x, chunk_y, chunk_z] = coord;

            // x and z are 24 bits, y is 16 bits
            // arrange like
            // MSB (XXX)(YY)(ZZZ) LSB
            uint64_t hash = static_cast<uint32_t>(chunk_x) & ~(~0u << 24);
            hash <<= 16;
            hash |= static_cast<uint32_t>(chunk_y) & ~(~0u << 16);
            hash <<= 24;
            hash |= static_cast<uint32_t>(chunk_z) & ~(~0u << 24);

            // David Stafford's Mix13 for MurmurHash3's 64-bit finalizer
            hash = (hash ^ (hash >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
            hash = (hash ^ (hash >> 27)) * UINT64_C(0x94D049BB133111EB);
            hash = hash ^ (hash >> 31);

            return hash;
        }
    };

    std::unordered_map<ChunkCoord, std::pair<ChunkStoreEntry, std::list<ChunkCoord>::const_iterator>, ChunkCoordHasher>
        map;
    std::list<ChunkCoord> lru;
    size_t max_size;

    ChunkStoreHandle operator=(const ChunkStoreHandle&) = delete;
    ChunkStoreHandle(const ChunkStoreHandle&) = delete;

public:
    ChunkStoreHandle(size_t max_size) : max_size(max_size) {}

    // Returns a pointer to the chunk at the given coordinates, if it is loaded, otherwise nullptr.
    // Does not mark the chunk as used for the LRU.
    // Assumes chunk is in valid range
    const ChunkStoreEntry* get(int chunk_x, int chunk_y, int chunk_z) const;

    // Returns a pointer to the chunk at the given coordinates, if it is loaded, otherwise nullptr.
    // Marks the chunk as used for the LRU.
    // Assumes chunk is in valid range
    ChunkStoreEntry* get_and_mark_used(int chunk_x, int chunk_y, int chunk_z);

    // Puts a chunk into the store, evicting the least recently used chunk if necessary.
    // Assumes chunk is in valid range
    void put(int chunk_x, int chunk_y, int chunk_z, const ChunkStoreEntry& entry);
};

// SAFETY: ChunkStore must outlive the thread pool!!
// A store for chunks, which can be loaded and unloaded.
// The least recently used chunk is unloaded when the store is full.
class ChunkStore {
    std::mutex mutex;
    ChunkStoreHandle handle;
    worldgen::ChunkGenerator<16, 16, 16> chunk_generator;

public:
    ChunkStore(size_t max_size, uint32_t worldgen_seed) : handle(max_size), chunk_generator(worldgen_seed) {}

    // Loads the chunks in a cube of side 2n+1 centred on the chunk if they are not already loaded, by sending the work
    // to the given thread pool.
    void load_n_around_on_pool(ThreadPool& pool, int chunk_x, int chunk_y, int chunk_z, int n);

    // Loads a chunk into the store, if it is not already loaded.
    // If the store is full, the least recently loaded chunk is unloaded.
    // If the chunk is already loaded, it is treated as if it was just loaded for the above purpose.
    // Assumes chunk is in valid range
    void load_chunk(int chunk_x, int chunk_y, int chunk_z);

    // Runs the given function with an exclusive handle to the chunk store.
    // Allows for multiple operations on the store to be performed.
    void use_handle(const std::function<void(ChunkStoreHandle&)>& f);
};

}  // namespace mgr