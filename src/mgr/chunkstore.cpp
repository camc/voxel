#include <mgr/chunkstore.h>
#include <config.h>
#include <iostream>
#include <functional>
#include <render/renderer.h>

using namespace mgr;

const ChunkStoreEntry* ChunkStoreHandle::get(int chunk_x, int chunk_y, int chunk_z) const {
    assert(chunk_x <= config::MAX_CHUNK_X && chunk_x >= config::MIN_CHUNK_X);
    // assert(chunk_y <= config::MAX_CHUNK_Y && chunk_y >= config::MIN_CHUNK_Y); - easy to trigger, other for debugging
    assert(chunk_z <= config::MAX_CHUNK_Z && chunk_z >= config::MIN_CHUNK_Z);

    const ChunkStoreHandle::ChunkCoord coord = {chunk_x, chunk_y, chunk_z};

    auto it = map.find(coord);

    if (it != map.end()) {
        return &it->second.first;
    } else {
        return nullptr;
    }
}

ChunkStoreEntry* ChunkStoreHandle::get_and_mark_used(int chunk_x, int chunk_y, int chunk_z) {
    assert(chunk_x <= config::MAX_CHUNK_X && chunk_x >= config::MIN_CHUNK_X);
    assert(chunk_y <= config::MAX_CHUNK_Y && chunk_y >= config::MIN_CHUNK_Y);
    assert(chunk_z <= config::MAX_CHUNK_Z && chunk_z >= config::MIN_CHUNK_Z);

    const ChunkStoreHandle::ChunkCoord coord = {chunk_x, chunk_y, chunk_z};

    auto it = map.find(coord);

    if (it != map.end()) {
        lru.splice(lru.begin(), lru, it->second.second);
        return &it->second.first;
    } else {
        return nullptr;
    }
}

void ChunkStoreHandle::put(int chunk_x, int chunk_y, int chunk_z, const ChunkStoreEntry& entry) {
    assert(chunk_x <= config::MAX_CHUNK_X && chunk_x >= config::MIN_CHUNK_X);
    assert(chunk_y <= config::MAX_CHUNK_Y && chunk_y >= config::MIN_CHUNK_Y);
    assert(chunk_z <= config::MAX_CHUNK_Z && chunk_z >= config::MIN_CHUNK_Z);

    const ChunkStoreHandle::ChunkCoord coord = {chunk_x, chunk_y, chunk_z};

    auto it = map.find(coord);
    if (it != map.end()) {
        map.erase(coord);
        lru.erase(it->second.second);
    } else if (lru.size() == max_size) {
        ChunkCoord last = lru.back();
        lru.pop_back();
        map.erase(last);
    }

    map.emplace(coord, std::make_pair(entry, lru.emplace(lru.begin(), coord)));
}

void ChunkStore::load_chunk(int chunk_x, int chunk_y, int chunk_z) {
    // TODO need to avoid loading/overwriting chunks that are already loaded

    ChunkStoreEntry entry = ChunkStoreEntry();
    chunk_generator.generate(entry.chunk, chunk_x, chunk_y, chunk_z);
    entry.instance_count =
        render::generate_chunk_vertex_data(entry.chunk, chunk_x, chunk_y, chunk_z, entry.vertex_data);

    std::scoped_lock<std::mutex> lock(mutex);
    handle.put(chunk_x, chunk_y, chunk_z, entry);
}

void ChunkStore::load_n_around_on_pool(ThreadPool& pool, int chunk_x, int chunk_y, int chunk_z, int n) {
    std::scoped_lock<std::mutex> lock(mutex);

    // Get jobs to load the chunks in each direction, including the chunk itself
    std::vector<std::function<void()>> jobs_todo;

    for (int dx = -n; dx <= n; dx++) {
        if (chunk_x + dx > config::MAX_CHUNK_X || chunk_x + dx < config::MIN_CHUNK_X) continue;

        for (int dy = -n; dy <= n; dy++) {
            if (chunk_y + dy > config::MAX_CHUNK_Y || chunk_y + dy < config::MIN_CHUNK_Y) continue;

            for (int dz = -n; dz <= n; dz++) {
                if (chunk_z + dz > config::MAX_CHUNK_Z || chunk_z + dz < config::MIN_CHUNK_Z) continue;

                int new_chunk_x = chunk_x + dx;
                int new_chunk_y = chunk_y + dy;
                int new_chunk_z = chunk_z + dz;

                ChunkStoreEntry* maybe_chunk = handle.get_and_mark_used(new_chunk_x, new_chunk_y, new_chunk_z);

                if (maybe_chunk == nullptr) {
                    jobs_todo.push_back([this, new_chunk_x, new_chunk_y, new_chunk_z] {
                        load_chunk(new_chunk_x, new_chunk_y, new_chunk_z);
                    });
                }
            }
        }
    }

    pool.enqueue(jobs_todo);
}

void ChunkStore::use_handle(const std::function<void(ChunkStoreHandle&)>& f) {
    std::scoped_lock<std::mutex> lock(mutex);
    f(handle);
}