#pragma once

#include "gfxm/gfxm.h"
#include <glad/glad.h>
#include <atomic>

class App {
    int _width, _height;
    double prev_mouse_x, prev_mouse_y;
    gfxm::Camera _camera;
    float _fov;
    bool camera_locked = false;

public:
    App(int width, int height, float fov) : _width(width), _height(height), _fov(fov) {}

    void set_framebuffer_size(int width, int height) {
        glViewport(0, 0, width, height);
        _width = width;
        _height = height;
    }

    // Sets the mouse pos used to calculate the mouse delta
    void set_prev_mouse_pos(double x, double y) {
        prev_mouse_x = x;
        prev_mouse_y = y;
    }

    void handle_mouse_move(double x, double y) {
        if (camera_locked) return;

        double sens = 0.01;

        double dx = x - prev_mouse_x;
        double dy = y - prev_mouse_y;

        float yaw = _camera.yaw() + dx * sens;
        float pitch = _camera.pitch() - dy * sens;
        pitch = std::clamp(pitch, -gfxm::HALF_PI_F * 0.98f, gfxm::HALF_PI_F * 0.98f);

        _camera.set_angles(pitch, yaw);

        this->set_prev_mouse_pos(x, y);
    }

    int width() const { return _width; }

    int height() const { return _height; }

    float fov() const { return _fov; }

    void toggle_camera_lock() { camera_locked = !camera_locked; }

    gfxm::Camera& camera() { return _camera; }

    const gfxm::Camera& camera() const { return _camera; }
};