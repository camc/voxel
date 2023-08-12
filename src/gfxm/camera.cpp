#include <gfxm/gfxm.h>

#include <iostream>

using namespace gfxm;

static constexpr Vec<3> CAMERA_WORLD_UP = VEC3_J;

// Produces a view transformration matrix for a camera looking in a given direction
// Direction must be a unit vector
static Matrix<4, 4> look_in_direction(const Matrix<3, 1>& camera_pos, const Matrix<3, 1>& direction,
                                      const Matrix<3, 1>& world_up) {
    assert(std::abs(direction.magnitude() - 1.0f) < 0.000001);

    Matrix<3, 1> ax_z = -direction;
    Matrix<3, 1> ax_x = direction.cross(VEC3_J).normalise();
    Matrix<3, 1> ax_y = ax_x.cross(direction).normalise();

    Matrix<3, 1> translation({-ax_x.dot(camera_pos), -ax_y.dot(camera_pos), -ax_z.dot(camera_pos)});

    return Matrix<4, 4>::from_columns({ax_x.appending_element(0.0f), ax_y.appending_element(0.0f),
                                       ax_z.appending_element(0.0f), Matrix<4, 1>({0.0f, 0.0f, 0.0f, 1.0f})})
        .transpose()
        .translate(translation);
}

// Produces a view transoformation matrix for a camera looking with a given pitch and yaw (radians)
// A pitch of 0 is horizontal, increasing pitch from 0 will cause the camera to look upwards.
// A yaw of 0 is towards +x, increasing yaw from 0 will rotate towards -z.
static Matrix<4, 4> look_pitch_yaw(const Matrix<3, 1>& camera_pos, float pitch_rad, float yaw_rad,
                                   const Matrix<3, 1>& world_up) {
    float sinpitch = sinf(pitch_rad);
    float cospitch = cosf(pitch_rad);
    float sinyaw = sinf(yaw_rad);
    float cosyaw = cosf(yaw_rad);

    Matrix<3, 1> direction({cosyaw * cospitch, sinpitch, cospitch * sinyaw});
    return look_in_direction(camera_pos, direction, world_up);
}

// TODO: better way of doing this
void Camera::move(const Matrix<3, 1>& translation) {
    Matrix<3, 1> to_world = Matrix<3, 3>(this->view()).transpose() * translation;
    _pos = _pos + to_world;
}

Matrix<4, 4> Camera::view() const { return look_pitch_yaw(_pos, _pitch, _yaw, CAMERA_WORLD_UP); }