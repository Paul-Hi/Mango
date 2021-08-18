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

bounding_frustum::bounding_frustum(const mat4& projection)
{
    // Calculate camera frustum in world space
    static vec3 points_hom[6] = {
        vec3(-1.0f, 0.0f, 1.0f), // (far) left
        vec3(1.0f, 0.0f, 1.0f),  // (far) right
        vec3(0.0f, -1.0f, 1.0f), // (far) bottom
        vec3(0.0f, 1.0f, 1.0f),  // (far) top

        vec3(0.0f, 0.0f, -1.0f), // near center
        vec3(0.0f, 0.0f, 1.0f),  // far center
    };

    vec3 points[6];
    mat4 inverse_projection = glm::inverse(projection);
    for (int32 i = 0; i < 6; ++i)
    {
        vec4 p    = inverse_projection * vec4(points_hom[i], 1.0f);
        points[i] = vec3(p / p.w);
    }

    origin   = vec3(0.0f);
    rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);

    z_near = points[4].z;
    z_far  = points[5].z;

    slope_left   = (points[0] / points[0].z).x;
    slope_right  = (points[1] / points[1].z).x;
    slope_bottom = (points[2] / points[2].z).y;
    slope_top    = (points[3] / points[3].z).y;
}

bounding_frustum bounding_frustum::get_transformed(mat4 transformation_matrix)
{
    bounding_frustum result;
    vec3 s;
    vec4 p;
    vec3 t;
    quat tr_rotation;
    vec3 tr_scale;
    glm::decompose(transformation_matrix, tr_scale, tr_rotation, t, s, p);

    result.rotation = rotation * tr_rotation;

    result.origin = vec3(transformation_matrix * vec4(origin, 1.0f));

    float scale = glm::compMax(tr_scale);

    result.z_near = z_near * scale;
    result.z_far  = z_far * scale;

    result.slope_left   = slope_left;
    result.slope_right  = slope_right;
    result.slope_bottom = slope_bottom;
    result.slope_top    = slope_top;

    return result;
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

std::array<vec3, 8> bounding_frustum::get_corners()
{
    vec4 stored[6] = {
        vec4(z_near),                                // near
        vec4(z_far),                                 // far
        vec4(slope_right, slope_top, 1.0f, 0.0f),    // top right
        vec4(slope_right, slope_bottom, 1.0f, 0.0f), // bottom right
        vec4(slope_left, slope_top, 1.0f, 0.0f),     // top left
        vec4(slope_left, slope_bottom, 1.0f, 0.0f),  // bottom left
    };

    std::array<vec3, 8> points = {
        stored[0] * stored[2], // top right near
        stored[0] * stored[3], // bottom right near
        stored[0] * stored[4], // top left near
        stored[0] * stored[5], // bottom left near
        stored[1] * stored[2], // top right far
        stored[1] * stored[3], // bottom right far
        stored[1] * stored[4], // top left far
        stored[1] * stored[5]  // bottom left far
    };

    for (int32 i = 0; i < 8; ++i)
    {
        points[i] = (rotation * points[i]) + origin;
    }

    return points;
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
    // setup frustum planes (normals point inwards)
    // x,y,z normal / w offset
    vec4 planes[6] = {
        vec4(0.0f, 0.0f, -1.0f, z_near),      // near
        vec4(0.0f, 0.0f, 1.0f, -z_far),       // far
        vec4(-1.0f, 0.0f, slope_right, 0.0f), // right
        vec4(1.0f, 0.0f, -slope_left, 0.0f),  // left
        vec4(0.0f, -1.0f, slope_top, 0.0f),   // top
        vec4(0.0f, 1.0f, -slope_bottom, 0.0f) // bottom
    };

    // Transform box to frustum space
    mat4 transformation_matrix             = glm::translate(glm::mat4_cast(glm::inverse(rotation)), -origin);
    axis_aligned_bounding_box bounding_box = other.get_transformed(transformation_matrix);
    auto corners                           = bounding_box.get_corners();

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

std::array<vec3, 8> axis_aligned_bounding_box::get_corners()
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
