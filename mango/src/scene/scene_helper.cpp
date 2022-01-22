//! \file      scene_helper.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <scene/scene_helper.hpp>

using namespace mango;

void mango::view_projection_perspective_camera(const perspective_camera& camera, const vec3& camera_position, mat4& out_view, mat4& out_projection)
{
    vec3 front = camera.target - camera_position;
    if (front.norm() > 1e-5)
    {
        front = front.normalized();
    }
    else
    {
        front = GLOBAL_FORWARD;
    }
    auto right = (GLOBAL_UP.cross(front)).normalized();
    auto up    = (front.cross(right)).normalized();

    out_view       = lookAt(camera_position, camera.target, up);
    out_projection = perspective(camera.vertical_field_of_view, camera.aspect, camera.z_near, camera.z_far);
}

void mango::view_projection_orthographic_camera(const orthographic_camera& camera, const vec3& camera_position, mat4& out_view, mat4& out_projection)
{
    vec3 front = camera.target - camera_position;
    if (front.norm() > 1e-5)
    {
        front = front.normalized();
    }
    else
    {
        front = GLOBAL_FORWARD;
    }
    auto right = (GLOBAL_UP.cross(front)).normalized();
    auto up    = (front.cross(right)).normalized();

    out_view       = lookAt(camera_position, camera.target, up);
    out_projection = ortho(-camera.x_mag * 0.5f, camera.x_mag * 0.5f, -camera.y_mag * 0.5f, camera.y_mag * 0.5f, camera.z_near, camera.z_far);
}