#include <worldgen/generator.h>
#include <FastNoise/FastNoise.h>
#include <iostream>
#include <config.h>
#include <numeric>
#include <models/block.h>

using namespace worldgen;

template <unsigned short X_SIZE, unsigned short Y_SIZE, unsigned short Z_SIZE>
ChunkGenerator<X_SIZE, Y_SIZE, Z_SIZE>::ChunkGenerator(uint32_t _seed) noexcept : seed(_seed) {
    FastNoise::SmartNode<> simplex = FastNoise::New<FastNoise::Simplex>();

    fbm_generator = FastNoise::New<FastNoise::FractalFBm>();

    fbm_generator->SetSource(simplex);
    fbm_generator->SetOctaveCount(4);
    fbm_generator->SetLacunarity(2.0f);
    fbm_generator->SetGain(0.5f);
};

template <unsigned short X_SIZE, unsigned short Y_SIZE, unsigned short Z_SIZE>
void ChunkGenerator<X_SIZE, Y_SIZE, Z_SIZE>::generate(models::Chunk<X_SIZE, Y_SIZE, Z_SIZE> &chunk, int chunk_x,
                                                      int chunk_y, int chunk_z) const {
    assert(chunk_x <= config::MAX_CHUNK_X && chunk_x >= config::MIN_CHUNK_X);
    assert(chunk_y <= config::MAX_CHUNK_Y && chunk_y >= config::MIN_CHUNK_Y);
    assert(chunk_z <= config::MAX_CHUNK_Z && chunk_z >= config::MIN_CHUNK_Z);

    // TODO either create a new chunk or clear the existing one?

    models::Block block = models::Block(models::STONE_BLOCK);
    if (chunk_y < config::MIN_CHUNK_Y + 2) {
        block = models::Block(models::DIRT_BLOCK);
    }

    std::vector<float> noise(static_cast<size_t>(X_SIZE) * Z_SIZE);
    fbm_generator->GenUniformGrid2D(noise.data(), chunk_x * X_SIZE, chunk_z * Z_SIZE, X_SIZE, Z_SIZE, 0.005f, seed);

    const float max_height = (float)config::MAX_CHUNK_Y * Y_SIZE;
    const float min_height = (float)config::MIN_CHUNK_Y * Y_SIZE;

    for (int z = 0; z < Z_SIZE; z++) {
        for (int x = 0; x < X_SIZE; x++) {
            float sample = noise[static_cast<size_t>(x) + static_cast<size_t>(z) * X_SIZE];
            float height = std::lerp(min_height, max_height, (sample + 1.0f) / 2.0f);

            int height_here = (int)std::floor(height) - chunk_y * Y_SIZE;

            for (int y = 0; y < height_here && y < Y_SIZE; y++) {
                chunk[x, y, z] = block;
            }
        }
    }
}

template class ChunkGenerator<16, 16, 16>;
