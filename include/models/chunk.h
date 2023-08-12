#pragma once

#include "block.h"

namespace models {

template <unsigned short _X_SIZE, unsigned short _Y_SIZE, unsigned short _Z_SIZE>
class Chunk {
    std::array<Block, _X_SIZE * _Y_SIZE * _Z_SIZE> blocks;

public:
    static constexpr unsigned short X_SIZE = _X_SIZE;
    static constexpr unsigned short Y_SIZE = _Y_SIZE;
    static constexpr unsigned short Z_SIZE = _Z_SIZE;

    Chunk() noexcept { blocks.fill(Block(EMPTY_BLOCK)); }

    const Block& operator[](unsigned int x, unsigned int y, unsigned int z) const {
        assert(x < X_SIZE && y < Y_SIZE && z < Z_SIZE);
        return blocks[x + y * X_SIZE + z * X_SIZE * Y_SIZE];
    }

    Block& operator[](unsigned int x, unsigned int y, unsigned int z) {
        assert(x < X_SIZE && y < Y_SIZE && z < Z_SIZE);
        return blocks[x + y * X_SIZE + z * X_SIZE * Y_SIZE];
    }
};

using RenderingChunk = Chunk<16, 16, 16>;

}  // namespace models