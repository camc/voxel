#pragma once

#include <glad/glad.h>

void GLAPIENTRY debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                       const char *message, const void *userParam);

void debug_log(const char *message);

inline constexpr bool debug_enabled() {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}