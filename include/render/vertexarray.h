#pragma once

#include <glad/glad.h>
#include <span>

namespace render {

class VertexArray {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    std::size_t m_indices_count;

    VertexArray(const VertexArray &) = delete;
    VertexArray &operator=(const VertexArray &) = delete;

public:
    using INDICES_TYPE = uint8_t;
    static constexpr GLenum INDICES_TYPE_GL = GL_UNSIGNED_BYTE;

    struct VertexAttribute {
        GLenum type;
        GLuint index;
        GLint size;
        GLsizei stride;
        const GLvoid *pointer;
        GLuint divisor;
    };

    VertexArray(std::span<const float> data, std::span<const INDICES_TYPE> indices,
                std::span<const VertexAttribute> attributes) noexcept;

    VertexArray() noexcept : VertexArray({}, {}, {}) {}

    ~VertexArray() {
        glDeleteBuffers(1, &this->vbo);
        glDeleteBuffers(1, &this->ebo);
        glDeleteVertexArrays(1, &this->vao);
    }

    void bind() { glBindVertexArray(this->vao); }

    void draw() {
        this->bind();
        glDrawElements(GL_TRIANGLES, this->indices_count(), INDICES_TYPE_GL, 0);
    }

    void draw_instanced(unsigned int count) {
        this->bind();
        glDrawElementsInstanced(GL_TRIANGLES, this->indices_count(), INDICES_TYPE_GL, 0, count);
    }

    void set_data(std::span<const uint8_t> data) {
        this->bind();
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
    }

    std::size_t indices_count() const { return this->m_indices_count; }
};

}  // namespace render