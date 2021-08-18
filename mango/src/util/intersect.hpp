//! \file      intersect.hpp
//! This file provides structures and functions to use for intersections.
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_INTERSECT_HPP
#define MANGO_INTERSECT_HPP

//! \cond NO_COND
#define GLM_FORCE_SILENT_WARNINGS 1
//! \endcond
#include <array>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <mango/types.hpp>

namespace mango
{
    // Interesting: https://github.com/gszauer/GamePhysicsCookbook

    //! \brief The result of any contains(...) check.
    enum containment_result : uint8
    {
        disjoint = 0,
        intersect,
        contain,
        count
    };

    struct bounding_frustum;
    struct axis_aligned_bounding_box;

    //! \brief Bounding sphere.
    struct bounding_sphere
    {
      public:
        //! \brief Constructs a \a bounding_sphere.
        //! \param[in] sphere_center The center of the new \a bounding_sphere.
        //! \param[in] sphere_radius The radius of the new \a bounding_sphere.
        bounding_sphere(const vec3& sphere_center, float sphere_radius)
            : center(sphere_center)
            , radius(sphere_radius)
        {
        }
        bounding_sphere()
            : center(0.0f)
            , radius(0.0f)
        {
        }

        //! \brief Checks the intersection of this \a bounding_sphere with another one.
        //! \return True, if the two \a bounding_spheres intersect, else false.
        bool intersects(const bounding_sphere& other) const;
        //! \brief Checks the intersection of this \a bounding_sphere with a \a bounding_frustum.
        //! \return True, if the \a bounding_sphere and the \a bounding_frustum intersect, else false.
        bool intersects(const bounding_frustum& other) const;

        //! \brief The center point of the \a bounding_sphere.
        vec3 center;
        //! \brief The radius of the \a bounding_sphere.
        float radius;
    };

    //! \brief Bounding frustum.
    //! \details Can also be used as a 'normal' frustum structure.
    struct bounding_frustum
    {
      public:
        //! \brief Constructs a \a bounding_frustum.
        //! \param[in] frustum_planes The six bounding planes of the new \a bounding_frustum.
        bounding_frustum(const std::array<vec4, 6>& frustum_planes)
            : planes(frustum_planes)
        {
        }
        //! \brief Constructs a \a bounding_frustum from a view projection matrix.
        //! \param[in] view The view matrix to construct the \a bounding_frustum from.
        //! \param[in] projection The projection matrix to construct the \a bounding_frustum from.
        bounding_frustum(const mat4& view, const mat4& projection);
        bounding_frustum() = default;

        //! \brief Retrieves frustum corner points for a given view projection matrix.
        //! \param[in] view_projection The view projection matrix to use.
        //! \return The corner points.
        static std::array<vec3, 8> get_corners(const mat4& view_projection);

        //! \brief Checks the intersection of this \a bounding_frustum with a \a bounding_sphere.
        //! \return True, if the \a bounding_frustum and the \a bounding_sphere intersect, else false.
        bool intersects(const bounding_sphere& other) const;
        //! \brief Checks the intersection of this \a bounding_frustum with a \a axis_aligned_bounding_box.
        //! \return True, if the \a bounding_frustum and the \a axis_aligned_bounding_box intersect, else false.
        bool intersects(const axis_aligned_bounding_box& other) const;

        //! \brief Planes of the frustum.
        //! \details Planes are: x,y,z = normal pointing inwards / w = offset to (0,0,0).
        std::array<vec4, 6> planes;
    };

    //! \brief An axis aligned bounding box.
    struct axis_aligned_bounding_box
    {
      public:
        //! \brief Constructs an \a axis_aligned_bounding_box given the center and the extends.
        //! \param[in] center_point The center of the new \a axis_aligned_bounding_box.
        //! \param[in] box_extents The extents of the new \a axis_aligned_bounding_box.
        axis_aligned_bounding_box(const vec3& center_point, const vec3& box_extents)
            : center(center_point)
            , extents(box_extents)
        {
        }
        axis_aligned_bounding_box()
            : center(0.0f)
            , extents(0.0f)
        {
        }

        //! \brief Creates an \a axis_aligned_bounding_box given minimum and maximum points.
        //! \param[in] min_point The smallest point included by the new \a axis_aligned_bounding_box.
        //! \param[in] max_point The biggest point included by the new \a axis_aligned_bounding_box.
        static axis_aligned_bounding_box from_min_max(const vec3& min_point, const vec3& max_point);
        //! \brief Creates and returns a transformed \a axis_aligned_bounding_box.
        //! \details Does only transform the box! No recalculation based on transformed enclosed geometry is done here!
        //! \param[in] transformation_matrix The transformation to transform the \a axis_aligned_bounding_box.
        //! \return A transformed \a axis_aligned_bounding_box.
        axis_aligned_bounding_box get_transformed(mat4 transformation_matrix) const;
        //! \brief Retrieves the corner points of the \a axis_aligned_bounding_box.
        //! \return The corner points.
        std::array<vec3, 8> get_corners() const;

        //! \brief Checks the intersection of this \a axis_aligned_bounding_box with another one.
        //! \return True, if the two \a axis_aligned_bounding_boxes intersect, else false.
        bool intersects(const axis_aligned_bounding_box& other) const;
        //! \brief Checks the intersection of this \a axis_aligned_bounding_box with a \a bounding_frustum.
        //! \return True, if the \a axis_aligned_bounding_box and the \a bounding_frustum intersect, else false.
        bool intersects(const bounding_frustum& other) const;

        //! \brief The center point of the \a axis_aligned_bounding_box.
        vec3 center;
        //! \brief The extents of the \a axis_aligned_bounding_box.
        vec3 extents;
    };
} // namespace mango

#endif // MANGO_HELPERS_HPP
