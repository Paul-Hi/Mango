//! \file      intersect.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <util/intersect.hpp>

using namespace mango;

bool bounding_sphere::intersects(const bounding_sphere& other) const
{
    return glm::distance(other.center, center) <= (other.radius + radius);
}

bool bounding_sphere::intersects(const bounding_frustum& other) const
{
    return other.intersects(*this);
}

bounding_frustum::bounding_frustum(const mat4& view, const mat4& projection)
{
    // Gribb/Hartmann
    mat4 combined = glm::transpose(projection * view);
    // Left
    planes[0] = combined[3] + combined[0];
    // Right
    planes[1] = combined[3] - combined[0];
    // Top
    planes[2] = combined[3] - combined[1];
    // Bottom
    planes[3] = combined[3] + combined[1];
    // Near
    planes[4] = combined[3] + combined[2];
    // Far
    planes[5] = combined[3] - combined[2];

    float len0 = glm::length(vec3(planes[0].x, planes[0].y, planes[0].z));
    float len1 = glm::length(vec3(planes[1].x, planes[1].y, planes[1].z));
    float len2 = glm::length(vec3(planes[2].x, planes[2].y, planes[2].z));
    float len3 = glm::length(vec3(planes[3].x, planes[3].y, planes[3].z));
    float len4 = glm::length(vec3(planes[4].x, planes[4].y, planes[4].z));
    float len5 = glm::length(vec3(planes[5].x, planes[5].y, planes[5].z));

    planes[0] /= len0;
    planes[1] /= len1;
    planes[2] /= len2;
    planes[3] /= len3;
    planes[4] /= len4;
    planes[5] /= len5;
}

std::array<vec3, 8> bounding_frustum::get_corners(const mat4& view_projection)
{
    // Homogenous points
    static vec4 hom[8] = {
        { 1.0f, 1.0f, 0.0f, 1.0f },   // top right near
        { 1.0f, -1.0f, 0.0f, 1.0f },  // bottom right near
        { -1.0f, 1.0f, 0.0f, 1.0f },  // top left near
        { -1.0f, -1.0f, 0.0f, 1.0f }, // bottom left near
        { 1.0f, 1.0f, 1.0f, 1.0f },   // top right far
        { 1.0f, -1.0f, 1.0f, 1.0f },  // bottom right far
        { -1.0f, 1.0f, 1.0f, 1.0f },  // top left far
        { -1.0f, -1.0f, 1.0f, 1.0f }  // bottom left far
    };
    std::array<vec4, 8> intermediate;
    std::array<vec3, 8> points;

    mat4 inverse = glm::inverse(view_projection);

    for (int i = 0; i < 8; i++)
    {
        intermediate[i] = inverse * hom[i];

        points[i] = vec3(intermediate[i] / intermediate[i].w);
    }

    return points;
}

bool bounding_frustum::intersects(const bounding_sphere& other) const
{
    for (int32 i = 0; i < 6; ++i)
    {
        if (dot(planes[i], vec4(other.center, 1.0f)) < -other.radius)
        {
            return false;
        }
    }

    return true;
}

bool bounding_frustum::intersects(const axis_aligned_bounding_box& other) const
{
    auto corners = other.get_corners();

    for (int32 i = 0; i < 6; ++i)
    {
        bool inside = false;
        for (int j = 0; j < 8; ++j)
        {
            if (dot(planes[i], vec4(corners[j], 1.0f)) >= 0.0f)
            {
                inside = true;
                break;
            }
        }

        if (!inside)
            return false;
    }

    return true;
}

axis_aligned_bounding_box axis_aligned_bounding_box::from_min_max(const vec3& min_point, const vec3& max_point)
{
    axis_aligned_bounding_box result;
    result.center  = (max_point + min_point) * 0.5f;
    result.extents = (max_point - min_point) * 0.5f;
    return result;
}

axis_aligned_bounding_box axis_aligned_bounding_box::get_transformed(mat4 transformation_matrix) const
{
    // computing the corner points of the aabb
    vec4 points[8] = {
        vec4(center + vec3(extents.x, extents.y, extents.z), 1.0f),   //
        vec4(center + vec3(extents.x, extents.y, -extents.z), 1.0f),  //
        vec4(center + vec3(extents.x, -extents.y, extents.z), 1.0f),  //
        vec4(center + vec3(extents.x, -extents.y, -extents.z), 1.0f), //
        vec4(center + vec3(-extents.x, extents.y, extents.z), 1.0f),  //
        vec4(center + vec3(-extents.x, extents.y, -extents.z), 1.0f), //
        vec4(center + vec3(-extents.x, -extents.y, extents.z), 1.0f), //
        vec4(center + vec3(-extents.x, -extents.y, -extents.z), 1.0f) //
    };

    vec3 min_value, max_value;
    min_value = max_value = vec3(transformation_matrix * points[0]);

    // transform points
    for (int32 i = 1; i < 8; ++i)
    {
        vec3 transformed = vec3(transformation_matrix * points[i]);

        min_value = glm::min(min_value, transformed);
        max_value = glm::max(max_value, transformed);
    }

    return axis_aligned_bounding_box::from_min_max(min_value, max_value);
}

std::array<vec3, 8> axis_aligned_bounding_box::get_corners() const
{
    std::array<vec3, 8> points = {
        center + vec3(extents.x, extents.y, extents.z),   //
        center + vec3(extents.x, extents.y, -extents.z),  //
        center + vec3(extents.x, -extents.y, extents.z),  //
        center + vec3(extents.x, -extents.y, -extents.z), //
        center + vec3(-extents.x, extents.y, extents.z),  //
        center + vec3(-extents.x, extents.y, -extents.z), //
        center + vec3(-extents.x, -extents.y, extents.z), //
        center + vec3(-extents.x, -extents.y, -extents.z) //
    };

    return points;
}

bool axis_aligned_bounding_box::intersects(const axis_aligned_bounding_box& other) const
{
    return !glm::any(glm::greaterThan(glm::abs(center - other.center), extents + other.extents));
}

bool axis_aligned_bounding_box::intersects(const bounding_frustum& other) const
{
    return other.intersects(*this);
}
