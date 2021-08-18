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
#include <mango/types.hpp>

namespace mango
{
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
        //! \brief Checks the intersection of this \a bounding_sphere with an \a axis_aligned_bounding_box.
        //! \return True, if the \a bounding_sphere and the \a axis_aligned_bounding_box intersect, else false.
        bool intersects(const axis_aligned_bounding_box& other) const;

        //! \brief Checks if this \a bounding_sphere contains another one.
        //! \return The \a containment_result of the query.
        containment_result contains(const bounding_sphere& other) const;
        //! \brief Checks if this \a bounding_sphere contains a \a bounding_frustum.
        //! \return The \a containment_result of the query.
        containment_result contains(const bounding_frustum& other) const;
        //! \brief Checks if this \a bounding_sphere contains an \a axis_aligned_bounding_box.
        //! \return The \a containment_result of the query.
        containment_result contains(const axis_aligned_bounding_box& other) const;

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
        //! \param[in] near_z Near plane z value.
        //! \param[in] far_z Fat plane z value.
        //! \param[in] right_slope Slope from near to far in +x direction.
        //! \param[in] left_slope Slope from near to far in -x direction.
        //! \param[in] bottom_slope Slope from near to far in +y direction
        //! \param[in] top_slope Slope from near to far in -y direction.
        //! \param[in] frustum_origin The origin of the new \a bounding_frustum.
        //! \param[in] frustum_rotation The rotation of the new \a bounding_frustum.
        bounding_frustum(float near_z, float far_z, float right_slope, float left_slope, float bottom_slope, float top_slope, const vec3& frustum_origin, const quat& frustum_rotation)
            : z_near(near_z)
            , z_far(far_z)
            , slope_right(right_slope)
            , slope_left(left_slope)
            , slope_bottom(bottom_slope)
            , slope_top(top_slope)
            , origin(frustum_origin)
            , rotation(frustum_rotation)
        {
        }
        //! \brief Constructs a \a bounding_frustum from a projection matrix.
        //! \details Matrix must not include any translation, rotation or scale, only a projection.
        //! \param[in] projection The projection matrix to construct the \a bounding_frustum from.
        bounding_frustum(const mat4& projection);
        bounding_frustum()
            : z_near(0.0f)
            , z_far(0.0f)
            , slope_right(0.0f)
            , slope_left(0.0f)
            , slope_bottom(0.0f)
            , slope_top(0.0f)
            , origin(0.0f)
            , rotation(1.0f, 0.0f, 0.0f, 0.0f)
        {
        }

        //! \brief Creates and returns a transformed \a bounding_frustum.
        //! \details Does only transform the box! No recalculation based on transformed enclosed geometry is done here!
        //! \param[in] transformation_matrix The transformation to transform the \a bounding_frustum.
        //! \return A transformed \a bounding_frustum.
        bounding_frustum get_transformed(mat4 transformation_matrix);
        //! \brief Retrieves the \a bounding_sphere of the \a bounding_frustum.
        //! \return The \a bounding_sphere enclosing the \a bounding_frustum.
        bounding_sphere get_bounding_sphere();
        //! \brief Retrieves the \a axis_aligned_bounding_box of the \a bounding_frustum.
        //! \return The \a axis_aligned_bounding_box enclosing the \a bounding_frustum.
        axis_aligned_bounding_box get_axis_aligned_bounding_box();
        //! \brief Retrieves center point of the \a bounding_frustum.
        //! \return The center point.
        vec3 get_center();
        //! \brief Retrieves the corner points of the \a bounding_frustum.
        //! \return The corner points.
        std::array<vec3, 8> get_corners();

        //! \brief Checks the intersection of this \a bounding_frustum with another one.
        //! \return True, if the two \a bounding_frustums intersect, else false.
        bool intersects(const bounding_frustum& other) const;
        //! \brief Checks the intersection of this \a bounding_frustum with a \a bounding_sphere.
        //! \return True, if the \a bounding_frustum and the \a bounding_sphere intersect, else false.
        bool intersects(const bounding_sphere& other) const;
        //! \brief Checks the intersection of this \a bounding_frustum with a \a axis_aligned_bounding_box.
        //! \return True, if the \a bounding_frustum and the \a axis_aligned_bounding_box intersect, else false.
        bool intersects(const axis_aligned_bounding_box& other) const;
        //! \brief Checks the intersection of this \a bounding_frustum with a \a axis_aligned_bounding_box.
        //! \details The fast version that is not 100% accurate.
        //! \return True, if the \a bounding_frustum and the \a axis_aligned_bounding_box intersect, else false.
        bool intersects_fast(const axis_aligned_bounding_box& other) const;

        //! \brief Checks if this \a bounding_frustum contains another one.
        //! \return The \a containment_result of the query.
        containment_result contains(const bounding_frustum& other) const;
        //! \brief Checks if this \a bounding_frustum contains a \a bounding_sphere.
        //! \return The \a containment_result of the query.
        containment_result contains(const bounding_sphere& other) const;
        //! \brief Checks if this \a bounding_frustum contains a \a axis_aligned_bounding_box.
        //! \return The \a containment_result of the query.
        containment_result contains(const axis_aligned_bounding_box& other) const;

        //! \brief Near plane z value.
        float z_near;
        //! \brief Far plane z value.
        float z_far;
        //! \brief Slope from near to far in +x direction.
        float slope_left;
        //! \brief Slope from near to far in -x direction.
        float slope_right;
        //! \brief Slope from near to far in +y direction.
        float slope_top;
        //! \brief Slope from near to far in -y direction.
        float slope_bottom;

        //! \brief Origin of the frustum.
        vec3 origin;
        //! \brief Rotation of the frustum.
        quat rotation;
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
        std::array<vec3, 8> get_corners();

        //! \brief Checks the intersection of this \a axis_aligned_bounding_box with another one.
        //! \return True, if the two \a axis_aligned_bounding_boxes intersect, else false.
        bool intersects(const axis_aligned_bounding_box& other) const;
        //! \brief Checks the intersection of this \a axis_aligned_bounding_box with a \a bounding_sphere.
        //! \return True, if the \a axis_aligned_bounding_box and the \a bounding_sphere intersect, else false.
        bool intersects(const bounding_sphere& other) const;
        //! \brief Checks the intersection of this \a axis_aligned_bounding_box with a \a bounding_frustum.
        //! \return True, if the \a axis_aligned_bounding_box and the \a bounding_frustum intersect, else false.
        bool intersects(const bounding_frustum& other) const;

        //! \brief Checks if this \a axis_aligned_bounding_box contains another one.
        //! \return The \a containment_result of the query.
        containment_result contains(const axis_aligned_bounding_box& other) const;
        //! \brief Checks if this \a axis_aligned_bounding_box contains a \a bounding_sphere.
        //! \return The \a containment_result of the query.
        containment_result contains(const bounding_sphere& other) const;
        //! \brief Checks if this \a axis_aligned_bounding_box contains a \a bounding_frustum.
        //! \return The \a containment_result of the query.
        containment_result contains(const bounding_frustum& other) const;

        //! \brief The center point of the \a axis_aligned_bounding_box.
        vec3 center;
        //! \brief The extents of the \a axis_aligned_bounding_box.
        vec3 extents;
    };
} // namespace mango

#endif // MANGO_HELPERS_HPP
