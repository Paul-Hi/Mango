//! \file      intersect.hpp
//! This file provides structures and functions to use for intersections.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_INTERSECT_HPP
#define MANGO_INTERSECT_HPP

/*
//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <glm/gtx/quaternion.hpp>
#include <mango/types.hpp>
*/

namespace mango
{
    /*
    struct bounding_sphere
    {
        vec3 center;
        float radius;

        bounding_sphere(const vec3& sphere_center, float sphere_radius)
            : center(sphere_center)
            , radius(sphere_radius)
        {
        }

        const vec3& get_center();
        float get_radius();
    };

    struct bounding_frustum
    {
        float near;   // offset of the plane from the world center in +z direction.
        float far;    // offset of the plane from the world center in -z direction.
        float right;  // offset of the plane from the world center in -x direction.
        float left;   // offset of the plane from the world center in +x direction.
        float bottom; // offset of the plane from the world center in -y direction.
        float top;    // offset of the plane from the world center in +y direction.

        vec3 center;   // center of the frustum.
        glm::quat rotation; // rotation of the frustum.

        bounding_frustum(float near_offset, float far_offset, float right_offset, float left_offset, float bottom_offset, float top_offset, const vec3& frustum_center,
                         const glm::quat& frustum_rotation)
            : near(near_offset)
            , far(far_offset)
            , right(right_offset)
            , left(left_offset)
            , bottom(bottom_offset)
            , top(top_offset)
            , center(frustum_center)
            , rotation(frustum_rotation)
        {
        }
        bounding_frustum(const mat4 view_projection);

        bounding_sphere get_bounding_sphere();
        axis_aligned_bounding_box get_axis_aligned_bounding_box();
        const vec3& get_center();
        vec3 get_corner(int32 i);
    };

    struct axis_aligned_bounding_box
    {
        vec3 minimum;
        vec3 maximum;

        axis_aligned_bounding_box(const vec3& min_point, const vec3& max_point)
            : minimum(min_point)
            , maximum(max_point)
        {
        }
        axis_aligned_bounding_box(vec3 center, vec3 extends);

        axis_aligned_bounding_box get_transformed(mat4 transform_matrix);
        vec3 get_center();
        vec3 get_halfwidth();
        float get_radius();
        vec3 get_corner(int32 i);
    }
    */
} // namespace mango

#endif // MANGO_HELPERS_HPP
