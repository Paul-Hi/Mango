//! \file      intersect.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2022
//! \copyright Apache License 2.0

#include <mango/intersect.hpp>

using namespace mango;

bool bounding_sphere::intersects(const bounding_sphere& other) const
{
    return (other.center - center).norm() <= (other.radius + radius);
}

bool bounding_sphere::intersects(const bounding_frustum& other) const
{
    return other.intersects(*this);
}

bounding_frustum::bounding_frustum(const mat4& view, const mat4& projection)
{
    // Gribb/Hartmann
    mat4 combined = (projection * view).transpose();
    // Left
    planes[0] = combined.col(3) + combined.col(0);
    // Right
    planes[1] = combined.col(3) - combined.col(0);
    // Top
    planes[2] = combined.col(3) - combined.col(1);
    // Bottom
    planes[3] = combined.col(3) + combined.col(1);
    // Near
    planes[4] = combined.col(3) + combined.col(2);
    // Far
    planes[5] = combined.col(3) - combined.col(2);

    float len0 = (vec3(planes[0].x(), planes[0].y(), planes[0].z())).norm();
    float len1 = (vec3(planes[1].x(), planes[1].y(), planes[1].z())).norm();
    float len2 = (vec3(planes[2].x(), planes[2].y(), planes[2].z())).norm();
    float len3 = (vec3(planes[3].x(), planes[3].y(), planes[3].z())).norm();
    float len4 = (vec3(planes[4].x(), planes[4].y(), planes[4].z())).norm();
    float len5 = (vec3(planes[5].x(), planes[5].y(), planes[5].z())).norm();

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

    mat4 inverse = view_projection.inverse();

    for (int i = 0; i < 8; i++)
    {
        intermediate[i] = inverse * hom[i];

        points[i] = intermediate[i].head<3>() / intermediate[i].w();
    }

    return points;
}

bool bounding_frustum::intersects(const bounding_sphere& other) const
{
    for (int32 i = 0; i < 6; ++i)
    {
        if (planes[i].dot(vec4(other.center.x(), other.center.y(), other.center.z(), 1.0f)) < -other.radius)
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
            if (planes[i].dot(vec4(corners[j].x(), corners[j].y(), corners[j].z(), 1.0f)) >= 0.0f)
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
        vec4(center.x() + extents.x(), center.y() + extents.y(), center.z() + extents.z(), 1.0f), //
        vec4(center.x() + extents.x(), center.y() + extents.y(), center.z() - extents.z(), 1.0f), //
        vec4(center.x() + extents.x(), center.y() - extents.y(), center.z() + extents.z(), 1.0f), //
        vec4(center.x() + extents.x(), center.y() - extents.y(), center.z() - extents.z(), 1.0f), //
        vec4(center.x() - extents.x(), center.y() + extents.y(), center.z() + extents.z(), 1.0f), //
        vec4(center.x() - extents.x(), center.y() + extents.y(), center.z() - extents.z(), 1.0f), //
        vec4(center.x() - extents.x(), center.y() - extents.y(), center.z() + extents.z(), 1.0f), //
        vec4(center.x() - extents.x(), center.y() - extents.y(), center.z() - extents.z(), 1.0f)  //
    };

    vec3 min_value, max_value;
    min_value = max_value = (transformation_matrix * points[0]).head<3>();

    // transform points
    for (int32 i = 1; i < 8; ++i)
    {
        vec3 transformed = (transformation_matrix * points[i]).head<3>();

        min_value = min(min_value, transformed);
        max_value = max(max_value, transformed);
    }

    return axis_aligned_bounding_box::from_min_max(min_value, max_value);
}

std::array<vec3, 8> axis_aligned_bounding_box::get_corners() const
{
    std::array<vec3, 8> points = {
        vec3(center.x() + extents.x(), center.y() + extents.y(), center.z() + extents.z()), //
        vec3(center.x() + extents.x(), center.y() + extents.y(), center.z() - extents.z()), //
        vec3(center.x() + extents.x(), center.y() - extents.y(), center.z() + extents.z()), //
        vec3(center.x() + extents.x(), center.y() - extents.y(), center.z() - extents.z()), //
        vec3(center.x() - extents.x(), center.y() + extents.y(), center.z() + extents.z()), //
        vec3(center.x() - extents.x(), center.y() + extents.y(), center.z() - extents.z()), //
        vec3(center.x() - extents.x(), center.y() - extents.y(), center.z() + extents.z()), //
        vec3(center.x() - extents.x(), center.y() - extents.y(), center.z() - extents.z())  //
    };

    return points;
}

bool axis_aligned_bounding_box::intersects(const axis_aligned_bounding_box& other) const
{
    vec3 dif = abs(vec3(center - other.center));
    vec3 ext = extents + other.extents;
    if (dif.x() > ext.x())
        return false;
    if (dif.y() > ext.y())
        return false;
    if (dif.z() > ext.z())
        return false;
    return true;
}

bool axis_aligned_bounding_box::intersects(const bounding_frustum& other) const
{
    return other.intersects(*this);
}
