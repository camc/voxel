#pragma once

#include "vertexarray.h"
#include <vector>
#include "../app.h"
#include "../models/chunk.h"

namespace render {

class Renderer {
    VertexArray vertex_array;
    GLuint program;
    GLuint texture_array;
    GLint projview_uniform;
    GLint lightpos_uniform;
    std::vector<uint8_t> _vertex_data;
    unsigned int _instance_count;

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

public:
    Renderer() noexcept;
    ~Renderer();

    // Clear the added block vertex data ready for new data
    void reset();

    // Add block vertex data to the vertex data to be rendered
    void add_vertex_data(const std::span<const uint8_t> vertex_data, unsigned int instance_count);

    // Write the vertex data to the vertex array
    void write_vertex_data();

    // Render the vertex data
    void render(const App &app);
};

template <unsigned short X_SIZE, unsigned short Y_SIZE, unsigned short Z_SIZE>
unsigned int generate_chunk_vertex_data(const models::Chunk<X_SIZE, Y_SIZE, Z_SIZE> &chunk, int chunk_x, int chunk_y,
                                        int chunk_z, std::vector<uint8_t> &data);

}  // namespace render