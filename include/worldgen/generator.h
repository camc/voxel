#pragma once

#include "../models/chunk.h"
#include <FastNoise/FastNoise.h>

namespace worldgen {

template <unsigned short X_SIZE, unsigned short Y_SIZE, unsigned short Z_SIZE>
class ChunkGenerator {
    uint32_t seed;
    FastNoise::SmartNode<FastNoise::FractalFBm> fbm_generator;

public:
    ChunkGenerator(uint32_t seed) noexcept;

    void generate(models::Chunk<X_SIZE, Y_SIZE, Z_SIZE> &chunk, int chunk_x, int chunk_y, int chunk_z) const;
};

};  // namespace worldgen