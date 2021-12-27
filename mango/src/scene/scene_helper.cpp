//! \file      scene_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <scene/scene_helper.hpp>

using namespace mango;

static void view_projection_perspective_camera(const perspective_camera& camera, const vec3& camera_position, mat4& out_view, mat4& out_projection)
{
    vec3 front = camera.target - camera_position;
    if (glm::length(front) > 1e-5)
    {
        front = glm::normalize(front);
    }
    else
    {
        front = GLOBAL_FORWARD;
    }
    auto right = glm::normalize(glm::cross(GLOBAL_UP, front));
    auto up    = glm::normalize(glm::cross(front, right));

    out_view       = glm::lookAt(camera_position, camera.target, up);
    out_projection = glm::perspective(camera.vertical_field_of_view, camera.aspect, camera.z_near, camera.z_far);
}

static void view_projection_orthographic_camera(const orthographic_camera& camera, const vec3& camera_position, mat4& out_view, mat4& out_projection)
{
    vec3 front = camera.target - camera_position;
    if (glm::length(front) > 1e-5)
    {
        front = glm::normalize(front);
    }
    else
    {
        front = GLOBAL_FORWARD;
    }
    auto right = glm::normalize(glm::cross(GLOBAL_UP, front));
    auto up    = glm::normalize(glm::cross(front, right));

    out_view       = glm::lookAt(camera_position, camera.target, up);
    out_projection = glm::ortho(-camera.x_mag * 0.5f, camera.x_mag * 0.5f, -camera.y_mag * 0.5f, camera.y_mag * 0.5f, camera.z_near, camera.z_far);
}