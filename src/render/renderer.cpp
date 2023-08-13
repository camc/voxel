#include <render/renderer.h>
#include <render/image.h>
#include <config.h>
#include <iostream>
#include <tracy/Tracy.hpp>
#include <format>

using namespace render;

static constexpr std::array<float, 12> FACE_VERTS = {
    -1.0f, 1.0f,  -1.0f,  // top left
    -1.0f, -1.0f, -1.0f,  // bottom left
    1.0f,  1.0f,  -1.0f,  // top right
    1.0f,  -1.0f, -1.0f,  // bottom right
};

// ccw winding
static constexpr std::array<uint8_t, 6> FACE_INDICES = {
    0, 1, 2, 1, 3, 2,
};

// NOTE: all attributes must be at least 4 byte aligned...
static constexpr std::array<VertexArray::VertexAttribute, 6> build_chunk_render_attributes(
    unsigned long long verts_size) {
    constexpr GLsizei INSTANCED_STRIDE = 3 * sizeof(float) + 4 * sizeof(float);

    return {{
        // Vertices
        {.type = GL_FLOAT, .index = 0, .size = 3, .stride = 3 * sizeof(float), .pointer = 0, .divisor = 0},

        // Instanced

        // Position
        {.type = GL_FLOAT,
         .index = 1,
         .size = 3,
         .stride = INSTANCED_STRIDE,
         .pointer = (void *)(verts_size * sizeof(float)),
         .divisor = 1},

        // Rotation
        {.type = GL_FLOAT,
         .index = 2,
         .size = 1,
         .stride = INSTANCED_STRIDE,
         .pointer = (void *)(verts_size * sizeof(float) + 3 * sizeof(float)),
         .divisor = 1},

        // xScale
        {.type = GL_FLOAT,
         .index = 3,
         .size = 1,
         .stride = INSTANCED_STRIDE,
         .pointer = (void *)(verts_size * sizeof(float) + 3 * sizeof(float) + sizeof(float)),
         .divisor = 1},

        // yScale
        {.type = GL_FLOAT,
         .index = 4,
         .size = 1,
         .stride = INSTANCED_STRIDE,
         .pointer = (void *)(verts_size * sizeof(float) + 3 * sizeof(float) + 2 * sizeof(float)),
         .divisor = 1},

        // texID
        {.type = GL_FLOAT,
         .index = 5,
         .size = 1,
         .stride = INSTANCED_STRIDE,
         .pointer = (void *)(verts_size * sizeof(float) + 3 * sizeof(float) + 3 * sizeof(float)),
         .divisor = 1},
    }};
}

static const std::string vshader_src = std::format(R"(
#version 400 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 position;
layout (location = 2) in float rotation;
layout (location = 3) in float xScale;
layout (location = 4) in float yScale;
layout (location = 5) in float texID;

out vec3 texCoord;
out float lightCosine;

uniform mat4 projview;
uniform vec3 lightPos;

const mat4 ROTATIONS[6] = mat4[](
    // No rotation - front of cube
    mat4(   1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 1.0, 1.0
    ),

    // left of cube
    mat4(   0.0, 0.0, -1.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            1.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 1.0
    ),

    // back of cube
    mat4(   -1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, -1.0, 0.0,
            0.0, 0.0, 1.0, 1.0
    ),

    // right of cube
    mat4(   0.0, 0.0, 1.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            -1.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 1.0
    ),

    // bottom of cube
    mat4(   1.0, 0.0, 0.0, 0.0,
            0.0, 0.0, -1.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 1.0
    ),

    // top of cube
    mat4(   1.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 1.0
    )
);

const vec3 NORMALS_WORLD[6] = vec3[](
    vec3(0.0, 0.0, -1.0),    // front
    vec3(-1.0, 0.0, 0.0),   // left
    vec3(0.0, 0.0, 1.0),   // back
    vec3(1.0, 0.0, 0.0),    // right
    vec3(0.0, -1.0, 0.0),   // bottom
    vec3(0.0, 1.0, 0.0)     // top
);

const float HALF_BLOCK_SIZE = {} / 2;

void main() {{
    vec3 scaledVert = vec3(vertex.x * xScale, vertex.y * yScale, vertex.z);
    gl_Position = projview * (ROTATIONS[int(rotation)] * vec4(HALF_BLOCK_SIZE * scaledVert, 1.0) + vec4(position, 0.0));
    texCoord = vec3((scaledVert.x + xScale) / 2, (scaledVert.y + yScale) / 2, texID);
    lightCosine = max(dot(NORMALS_WORLD[int(rotation)], normalize(lightPos - position)), 0.0);
}}
)", config::BLOCK_SIZE);

static const char *fshader_src = R"(
#version 400 core

in vec3 texCoord;
in float lightCosine;
out vec4 colour;

uniform sampler2DArray tex;

void main() {
    const float ambient = 0.6;
    colour = texture(tex, texCoord) * (ambient + lightCosine / 2.5);
}
)";

struct __attribute__((packed)) VertexDataInstance {
    std::array<float, 3> position;
    float rotation;
    float xScale;
    float yScale;
    float texID;

    void append_to(std::vector<uint8_t> &vec) const { vec.insert(vec.end(), (uint8_t *)this, (uint8_t *)(this + 1)); }
};

static const std::array<VertexArray::VertexAttribute, 6> RENDER_ATTRIBUTES =
    build_chunk_render_attributes(FACE_VERTS.size());

enum class BlockRotation : unsigned int { FRONT = 0, LEFT = 1, BACK = 2, RIGHT = 3, BOTTOM = 4, TOP = 5 };

constexpr int dir_to_check(BlockRotation rot) {
    switch (rot) {
        case BlockRotation::FRONT:
            return -1;
        case BlockRotation::LEFT:
            return -1;
        case BlockRotation::BACK:
            return 1;
        case BlockRotation::RIGHT:
            return 1;
        case BlockRotation::BOTTOM:
            return -1;
        case BlockRotation::TOP:
            return 1;
        default:
            throw std::logic_error("Invalid block rotation");
    }
}

template <unsigned short X_SIZE, unsigned short Y_SIZE, unsigned short Z_SIZE>
unsigned int render::generate_chunk_vertex_data(const models::Chunk<X_SIZE, Y_SIZE, Z_SIZE> &chunk, int chunk_x,
                                                int chunk_y, int chunk_z, std::vector<uint8_t> &data) {
    data.clear();

    unsigned int instance_count = 0;

    // TODO: mesh rects of faces instead of just strips, refactor
    //       check neibouring chunks
    //         - need to hold lock?

    const gfxm::Vec<3> chunk_offset =
        gfxm::Vec<3>({(float)chunk_x * X_SIZE, (float)chunk_y * Y_SIZE, (float)chunk_z * Z_SIZE}) * config::BLOCK_SIZE;

    // Front and back
    for (int z = 0; z < Z_SIZE; z++) {
        for (int y = 0; y < Y_SIZE; y++) {
            for (const auto rot : {BlockRotation::FRONT, BlockRotation::BACK}) {
                const int dir = dir_to_check(rot);
                const int edge_z = dir == 1 ? Z_SIZE - 1 : 0;

                for (int x = 0; x < X_SIZE;) {
                    const models::Block &block = chunk[x, y, z];

                    if (block.id() == models::EMPTY_BLOCK) {
                        x++;
                        continue;
                    }

                    if (!(z == edge_z || !chunk[x, y, z + dir].opaque())) {
                        x++;
                        continue;
                    }

                    int x_start = x;
                    do {
                        x++;
                    } while (x < X_SIZE && chunk[x, y, z].id() == block.id() &&
                             (z == edge_z || !chunk[x, y, z + dir].opaque()));

                    float centre_x = (float)x_start + ((float)x - x_start) / 2.0f;

                    gfxm::Vec<3> pos = chunk_offset + gfxm::Vec<3>({centre_x, (float)y + 0.5f, (float)z + 0.5f}) *
                                                          (float)config::BLOCK_SIZE;

                    VertexDataInstance{.position = pos.array(),
                                       .rotation = (float)rot,
                                       .xScale = (float)(x - x_start),
                                       .yScale = 1.0f,
                                       .texID = (float)(block.id() - 1)}
                        .append_to(data);

                    instance_count++;
                }
            }
        }
    }

    // Left and right
    for (int y = 0; y < Y_SIZE; y++) {
        for (int x = 0; x < X_SIZE; x++) {
            for (const auto rot : {BlockRotation::LEFT, BlockRotation::RIGHT}) {
                const int dir = dir_to_check(rot);
                const int edge_x = dir == 1 ? X_SIZE - 1 : 0;

                for (int z = 0; z < Z_SIZE;) {
                    const models::Block &block = chunk[x, y, z];

                    if (block.id() == models::EMPTY_BLOCK) {
                        z++;
                        continue;
                    }

                    if (!(x == edge_x || !chunk[x + dir, y, z].opaque())) {
                        z++;
                        continue;
                    }

                    int z_start = z;
                    do {
                        z++;
                    } while (z < Z_SIZE && chunk[x, y, z].id() == block.id() &&
                             (x == edge_x || !chunk[x + dir, y, z].opaque()));

                    float centre_z = (float)z_start + ((float)z - z_start) / 2.0f;

                    gfxm::Vec<3> pos = chunk_offset + gfxm::Vec<3>({(float)x + 0.5f, (float)y + 0.5f, centre_z}) *
                                                          (float)config::BLOCK_SIZE;

                    VertexDataInstance{.position = pos.array(),
                                       .rotation = (float)rot,
                                       .xScale = (float)(z - z_start),
                                       .yScale = 1.0f,
                                       .texID = (float)(block.id() - 1)}
                        .append_to(data);

                    instance_count++;
                }
            }
        }
    }

    // Top and bottom
    for (int z = 0; z < Z_SIZE; z++) {
        for (int y = 0; y < Y_SIZE; y++) {
            for (const auto rot : {BlockRotation::BOTTOM, BlockRotation::TOP}) {
                const int dir = dir_to_check(rot);
                const int edge_y = dir == 1 ? Y_SIZE - 1 : 0;

                for (int x = 0; x < X_SIZE;) {
                    const models::Block &block = chunk[x, y, z];

                    if (block.id() == models::EMPTY_BLOCK) {
                        x++;
                        continue;
                    }

                    if (!(y == edge_y || !chunk[x, y + dir, z].opaque())) {
                        x++;
                        continue;
                    }

                    int x_start = x;
                    do {
                        x++;
                    } while (x < X_SIZE && chunk[x, y, z].id() == block.id() &&
                             (y == edge_y || !chunk[x, y + dir, z].opaque()));

                    float centre_x = (float)x_start + ((float)x - x_start) / 2.0f;

                    gfxm::Vec<3> pos = chunk_offset + gfxm::Vec<3>({centre_x, (float)y + 0.5f, (float)z + 0.5f}) *
                                                          (float)config::BLOCK_SIZE;

                    VertexDataInstance{.position = pos.array(),
                                       .rotation = (float)rot,
                                       .xScale = (float)(x - x_start),
                                       .yScale = 1.0f,
                                       .texID = (float)(block.id() - 1)}
                        .append_to(data);

                    instance_count++;
                }
            }
        }
    }

    return instance_count;
}

template unsigned int render::generate_chunk_vertex_data(
    const models::Chunk<models::RenderingChunk::X_SIZE, models::RenderingChunk::Y_SIZE, models::RenderingChunk::Z_SIZE>
        &chunk,
    int chunk_x, int chunk_y, int chunk_z, std::vector<uint8_t> &data);

Renderer::Renderer() noexcept : vertex_array({}, FACE_INDICES, RENDER_ATTRIBUTES) {
    // Load shaders
    program = glCreateProgram();

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    const char *vshader_cs = vshader_src.c_str();
    glShaderSource(vshader, 1, &vshader_cs, NULL);
    glCompileShader(vshader);

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fshader_src, NULL);
    glCompileShader(fshader);

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    projview_uniform = glGetUniformLocation(program, "projview");
    lightpos_uniform = glGetUniformLocation(program, "lightPos");

    // Load textures

    unsigned int img_width, img_height;
    unsigned int img2_width, img2_height;
    std::vector<char> img1 = load_png_rgb8("../assets/stone.png", img_width, img_height);
    std::vector<char> img2 = load_png_rgb8("../assets/dirt.png", img2_width, img2_height);

    assert(img_width == img2_width && img_height == img2_height);

    glGenTextures(1, &texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 4, GL_RGB8, img_width, img_height, 2);

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, img_width, img_height, 1, GL_RGB, GL_UNSIGNED_BYTE, img1.data());

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, img_width, img_height, 1, GL_RGB, GL_UNSIGNED_BYTE, img2.data());

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    // Add vertices
    _vertex_data.insert(_vertex_data.end(), (uint8_t *)FACE_VERTS.data(),
                        (uint8_t *)(FACE_VERTS.data() + FACE_VERTS.size()));
}

Renderer::~Renderer() {
    glDeleteProgram(program);
    glDeleteTextures(1, &texture_array);
}

void Renderer::reset() {
    _vertex_data.resize(FACE_VERTS.size() * sizeof(float));
    _instance_count = 0;
}

void Renderer::add_vertex_data(const std::span<const uint8_t> vertex_data, unsigned int instance_count) {
    _vertex_data.insert(_vertex_data.end(), vertex_data.begin(), vertex_data.end());
    _instance_count += instance_count;
}

void Renderer::write_vertex_data() { vertex_array.set_data(_vertex_data); }

void Renderer::render(const App &app) {
    ZoneScopedN("Renderer::render");

    static constexpr float near = 5.0f;
    static constexpr float far = 50000.0f;

    float aspect_ratio = (float)app.width() / (float)app.height();
    float half_near_height = near * tanf(app.fov() / 2.0f);
    float half_near_width = aspect_ratio * half_near_height;

    auto projection = gfxm::Matrix<4, 4>::from_rowmajor({near / half_near_width, 0, 0, 0.0f, 0, near / half_near_height,
                                                         0, 0.0f, 0, 0, -(far + near) / (far - near),
                                                         -2.0f * near * far / (far - near), 0, 0, -1.0f, 0.0f});

    auto projview = projection * app.camera().view();

    glUseProgram(program);
    glUniformMatrix4fv(projview_uniform, 1, GL_FALSE, projview.array().data());

    const auto pos = app.camera().pos();
    const float maxh = (float)config::BLOCK_SIZE * models::RenderingChunk::Y_SIZE * config::MAX_CHUNK_Y;
    auto light =
        gfxm::Vec<3>({pos[0, 0], maxh, pos[2, 0]}) +
        gfxm::Vec<3>({0.4755282581475768f, 0.8090169943749475f, 0.3454915028125263f}) * config::BLOCK_SIZE * 100;
    glUniform3f(lightpos_uniform, light[0, 0], light[1, 0], light[2, 0]);

    vertex_array.draw_instanced(_instance_count);
}