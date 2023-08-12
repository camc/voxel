#include <render/vertexarray.h>

using namespace render;

VertexArray::VertexArray(std::span<const float> data, std::span<const INDICES_TYPE> indices,
                         std::span<const VertexAttribute> attributes) noexcept {
    this->m_indices_count = indices.size();

    // Create VAO
    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    // Create VBO
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);

    // Set attributes, bind VBO to VAO
    for (const VertexAttribute& attr : attributes) {
        glVertexAttribPointer(attr.index, attr.size, attr.type, GL_FALSE, attr.stride, attr.pointer);
        glVertexAttribDivisor(attr.index, attr.divisor);
        glEnableVertexAttribArray(attr.index);
    }

    // Create EBO & bind to VAO
    glGenBuffers(1, &this->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), GL_STATIC_DRAW);
}