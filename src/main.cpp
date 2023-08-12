#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <span>
#include <debug.h>
#include <chrono>
#include <app.h>
#include <gfxm/gfxm.h>
#include <config.h>
#include <mgr/manager.h>
#include <mgr/sharedstate.h>
#include <render/renderer.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

enum class HandleInputResult {
    NONE,
    QUIT,  // The user wants to quit the game
    MOVED  // The player changeds position
};

static HandleInputResult handle_input(GLFWwindow *window, App &app, float delta_time) {
    glfwPollEvents();

    // Quit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) return HandleInputResult::QUIT;

    HandleInputResult result = HandleInputResult::NONE;

    // Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        app.camera().move(gfxm::Vec<3>({0, 0, -4000 * delta_time}));
        result = HandleInputResult::MOVED;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        app.camera().move(gfxm::Vec<3>({0, 0, 4000 * delta_time}));
        result = HandleInputResult::MOVED;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        app.camera().move(gfxm::Vec<3>({4000 * delta_time, 0, 0}));
        result = HandleInputResult::MOVED;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        app.camera().move(gfxm::Vec<3>({-4000 * delta_time, 0, 0}));
        result = HandleInputResult::MOVED;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        app.camera().set_pos(app.camera().pos() + gfxm::Vec<3>({0, 4000 * delta_time, 0}));
        result = HandleInputResult::MOVED;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        app.camera().set_pos(app.camera().pos() + gfxm::Vec<3>({0, -4000 * delta_time, 0}));
        result = HandleInputResult::MOVED;
    }

    // Reset camera direction
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        app.camera().set_angles(0.0f, gfxm::HALF_PI_F);
    }

    // Ungrab mouse
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        glfwSetInputMode(
            window, GLFW_CURSOR,
            glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        app.toggle_camera_lock();
    }

    return result;
}

static void glmain(GLFWwindow *window, App &app) {
    const GLubyte *gl_renderer = glGetString(GL_RENDERER);
    const GLubyte *gl_version = glGetString(GL_VERSION);
    std::cerr << gl_renderer << "\n" << gl_version << std::endl;

    // ----- State for all rendering
    {
        // Enable MSAA
        glEnable(GL_MULTISAMPLE);

        // Enable depth testing
        glEnable(GL_DEPTH_TEST);

        // Enable back face culling
        glEnable(GL_CULL_FACE);

        // The projection matrix changes the handednesss of the coordinate system.
        // Index buffer has CCW winding order, but Z changes direction after projection.
        glFrontFace(GL_CW);
    }
    // -----

    bool vsync = true;
    if (!vsync) {
        glfwSwapInterval(0);
    }

    app.camera().set_pos(gfxm::Vec<3>({0, 0, 0}));
    app.camera().set_angles(0.0f, gfxm::HALF_PI_F);

    std::chrono::time_point<std::chrono::steady_clock> last_frame_timestamp = std::chrono::steady_clock::now();

    uint32_t worldgen_seed =
        std::chrono::duration_cast<std::chrono::milliseconds>(last_frame_timestamp.time_since_epoch()).count();

    std::cout << "worldgen seed: " << worldgen_seed << std::endl;

    mgr::Manager manager{mgr::SharedStateView{
                             .chunk_x = 0,
                             .chunk_y = 0,
                             .chunk_z = 0,
                         },
                         worldgen_seed};

    render::Renderer renderer;

    bool should_regen_vertex_data = true;
    float delta_time = 0;

    while (!glfwWindowShouldClose(window)) {
        auto time = std::chrono::steady_clock::now();
        delta_time = std::chrono::duration<float>(time - last_frame_timestamp).count();
        last_frame_timestamp = time;

        HandleInputResult input_result = handle_input(window, app, delta_time);

        int chunk_x = std::floor(app.camera().pos()[0] / ((float)config::BLOCK_SIZE * models::RenderingChunk::X_SIZE));
        int chunk_y = std::floor(app.camera().pos()[1] / ((float)config::BLOCK_SIZE * models::RenderingChunk::Y_SIZE));
        int chunk_z = std::floor(app.camera().pos()[2] / ((float)config::BLOCK_SIZE * models::RenderingChunk::Z_SIZE));

        if (input_result == HandleInputResult::QUIT) {
            break;
        } else {
            manager.shared_state().modify([chunk_x, chunk_y, chunk_z](mgr::SharedStateView &state) {
                state.chunk_x = chunk_x;
                state.chunk_y = chunk_y;
                state.chunk_z = chunk_z;
            });

            // Changed chunk, so rendered chunks will be different
            should_regen_vertex_data = true;
        }

        if (should_regen_vertex_data) {
            ZoneScopedN("regen_vertex_data");

            should_regen_vertex_data = false;

            renderer.reset();

            manager.chunk_store().use_handle(
                [&renderer, &should_regen_vertex_data, chunk_x, chunk_y, chunk_z](const auto &chunk_store) {
                    ZoneScopedN("chunk_store_use");
                    for (int dx = -config::RENDER_DISTANCE; dx <= config::RENDER_DISTANCE; dx++) {
                        for (int dy = -config::RENDER_DISTANCE; dy <= config::RENDER_DISTANCE; dy++) {
                            for (int dz = -config::RENDER_DISTANCE; dz <= config::RENDER_DISTANCE; dz++) {
                                const mgr::ChunkStoreEntry *entry =
                                    chunk_store.get(chunk_x + dx, chunk_y + dy, chunk_z + dz);
                                if (entry != nullptr) {
                                    renderer.add_vertex_data(entry->vertex_data, entry->instance_count);
                                } else if (chunk_y + dy >= config::MIN_CHUNK_Y && chunk_y + dy <= config::MAX_CHUNK_Y) {
                                    // A chunk is not available yet
                                    should_regen_vertex_data = true;
                                }
                            }
                        }
                    }
                });

            renderer.write_vertex_data();
        }

        {
            TracyGpuZone("render");
            glClearColor(0.4f, 0.4f, 0.7f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.render(app);
        }

        {
            ZoneScopedN("swap_buffers");
            glfwSwapBuffers(window);
        }

        TracyGpuCollect;
        FrameMark;
    }
}

int main() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    constexpr int width = 1200;
    constexpr int height = 675;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

    // MSAA
    glfwWindowHint(GLFW_SAMPLES, 4);

    if (debug_enabled()) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    }

    GLFWwindow *window = glfwCreateWindow(width, height, "voxel", NULL, NULL);
    if (!window) {
        const char *desc;
        glfwGetError(&desc);
        glfwTerminate();
        throw std::runtime_error(std::string("glfwCreateWindow failed: ") + desc);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        glfwTerminate();
        throw std::runtime_error("gladLoadGL failed");
    }

    TracyGpuContext;

    if (debug_enabled()) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debug_message_callback, nullptr);
        debug_log("debug messages enabled");
    }

    App app(width, height, gfxm::HALF_PI_F * 0.7f);
    glfwSetWindowUserPointer(window, &app);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int w, int h) {
        App *w_app = (App *)glfwGetWindowUserPointer(window);
        w_app->set_framebuffer_size(w, h);
    });

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported()) glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    app.set_prev_mouse_pos(x, y);

    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
        App *w_app = (App *)glfwGetWindowUserPointer(window);
        w_app->handle_mouse_move(x, y);
    });

    glmain(window, app);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}