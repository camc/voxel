#pragma once

#include "../app.h"
#include "../models/chunk.h"
#include "../gfxm/matrix.h"
#include "vertexarray.h"

namespace render {

struct BlockVertexData {
    gfxm::Matrix<4, 4> model_mat;
    float texture_id;
};

class ChunkRenderer {
    VertexArray vertex_array;
    GLuint program;

    ChunkRenderer(const ChunkRenderer &) = delete;
    ChunkRenderer &operator=(const ChunkRenderer &) = delete;

public:
    ChunkRenderer() noexcept;
    // destructor??

    template <unsigned short X_SIZE, unsigned short Y_SIZE, unsigned short Z_SIZE>
    void render_chunk(const App &app, const models::Chunk<X_SIZE, Y_SIZE, Z_SIZE> &chunk, int chunkx, int chunkz);
};

}  // namespace render