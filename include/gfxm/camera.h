#pragma once

#include "matrix.h"

namespace gfxm {

class Camera {
    Vec<3> _pos;
    float _pitch;
    float _yaw;

public:
    constexpr Camera() noexcept : _pitch(0.0f), _yaw(0.0f){};

    Camera(const Vec<3>& camera_pos, float pitch_rad, float yaw_rad) noexcept {
        _pos = camera_pos;
        _pitch = pitch_rad;
        _yaw = yaw_rad;
    };

    // Translate the camera in camera space.
    void move(const Vec<3>& translation);

    // Set the pitch and yaw of the camera.
    void set_angles(float pitch_rad, float yaw_rad) {
        _pitch = pitch_rad;
        _yaw = yaw_rad;
    };

    // Generate the view matrix for the camera
    Matrix<4, 4> view() const;

    float pitch() const { return _pitch; }
    float yaw() const { return _yaw; }

    const Matrix<3, 1>& pos() const { return _pos; }

    // Set the camera position in world space
    void set_pos(const Matrix<3, 1>& pos) { _pos = pos; };
};

}  // namespace gfxm