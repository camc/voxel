#pragma once

#include <string>
#include "../gfxm/matrix.h"
#include <memory>

namespace models {

using BlockId = unsigned short;

struct BlockInfo {
    std::string name;
    bool opaque;
};

constexpr BlockId EMPTY_BLOCK = 0;
constexpr BlockId STONE_BLOCK = 1;
constexpr BlockId DIRT_BLOCK = 2;

constexpr std::array<BlockInfo, 3> BUILTIN_BLOCKS = {
    BlockInfo{.name = "empty", .opaque = false},
    BlockInfo{.name = "stone", .opaque = true},
    BlockInfo{.name = "dirt", .opaque = true},
};

class Block {
    BlockId _id;

    constexpr const BlockInfo& info() const { return BUILTIN_BLOCKS[_id]; }

public:
    constexpr Block() { _id = EMPTY_BLOCK; }

    constexpr Block(BlockId bid) {
        assert(bid < BUILTIN_BLOCKS.size());
        _id = bid;
    }

    constexpr BlockId id() const { return _id; }

    constexpr std::string name() const { return info().name; }

    constexpr bool opaque() const { return info().opaque; }
};

}  // namespace models
