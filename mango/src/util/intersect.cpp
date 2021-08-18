//! \file      intersect.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#include <glm/gtx/matrix_decompose.hpp>
#include <util/intersect.hpp>

using namespace mango;

bool bounding_sphere::intersects(const bounding_sphere& other) const
{
    return glm::distance(other.center, center) <= (other.radius + radius);
}

bool bounding_sphere::intersects(const bounding_frustum& other) const
{
    return false; // TODO
}

bool bounding_sphere::intersects(const axis_aligned_bounding_box& other) const
{
    return false; // TODO
}

containment_result bounding_sphere::contains(const bounding_sphere& other) const
{
    if (!intersects(other))
        return containment_result::disjoint;
    bool contains = radius >= other.radius;
    contains      = contains && glm::distance(other.center, center) < radius;
    return contains ? containment_result::contain : containment_result::intersect;
}

containment_result bounding_sphere::contains(const bounding_frustum& other) const
{
    return containment_result::disjoint; // TODO
}

containment_result bounding_sphere::contains(const axis_aligned_bounding_box& other) const
{
    return containment_result::disjoint; // TODO
}

bounding_frustum::bounding_frustum(const mat4& view, const mat4& projection)
{
    // Gribb/Hartmann
    mat4 combined = glm::transpose(projection * view);
    // Left
    planes[0] = glm::normalize(combined[3] + combined[0]);
    // Right
    planes[1] = glm::normalize(combined[3] - combined[0]);
    // Top
    planes[2] = glm::normalize(combined[3] - combined[1]);
    // Bottom
    planes[3] = glm::normalize(combined[3] + combined[1]);
    // Near
    planes[4] = glm::normalize(combined[3] + combined[2]);
    // Far
    planes[5] = glm::normalize(combined[3] - combined[2]);
}

bounding_sphere bounding_frustum::get_bounding_sphere()
{
    return bounding_sphere(); // TODO
}

axis_aligned_bounding_box bounding_frustum::get_axis_aligned_bounding_box()
{
    return axis_aligned_bounding_box(); // TODO
}

vec3 bounding_frustum::get_center()
{
    return vec3(0.0f); // TODO
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

    return points; // TODO
}

bool bounding_frustum::intersects(const bounding_frustum& other) const
{
    return false; // TODO
}

bool bounding_frustum::intersects(const bounding_sphere& other) const
{
    return false; // TODO
}

bool bounding_frustum::intersects(const axis_aligned_bounding_box& other) const
{
    return false; // TODO
}

bool bounding_frustum::intersects_fast(const axis_aligned_bounding_box& other) const
{
    auto corners = other.get_corners();

    // First check all planes
    for (int32 i = 0; i < 6; ++i)
    {
        bool inside = false;
        for (int j = 0; j < 8; ++j)
        {
            if (dot(planes[i], vec4(corners[j], 1.0f)) > 0.0f)
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

containment_result bounding_frustum::contains(const bounding_frustum& other) const
{
    return containment_result::disjoint; // TODO
}

containment_result bounding_frustum::contains(const bounding_sphere& other) const
{
    return containment_result::disjoint; // TODO
}

containment_result bounding_frustum::contains(const axis_aligned_bounding_box& other) const
{
    return containment_result::disjoint; // TODO
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
    return false; // TODO
}

bool axis_aligned_bounding_box::intersects(const bounding_sphere& other) const
{
    return false; // TODO
}

bool axis_aligned_bounding_box::intersects(const bounding_frustum& other) const
{
    return other.intersects(*this);
}

containment_result axis_aligned_bounding_box::contains(const axis_aligned_bounding_box& other) const
{
    return containment_result::disjoint; // TODO
}

containment_result axis_aligned_bounding_box::contains(const bounding_sphere& other) const
{
    return containment_result::disjoint; // TODO
}

containment_result axis_aligned_bounding_box::contains(const bounding_frustum& other) const
{
    return containment_result::disjoint; // TODO
}
