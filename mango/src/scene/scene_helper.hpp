//! \file      scene_helper.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2021
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_HELPER_HPP
#define MANGO_SCENE_HELPER_HPP

#include <mango/scene_structures.hpp>

namespace mango
{
    //! \brief Creates view and projection matrix from a given \a perspective_camera.
    //! \param[in] camera The \a perspective_camera to create the matrices from.
    //! \param[in] camera_position The position of the camera.
    //! \param[out] out_view The matrix to write the view matrix to.
    //! \param[out] out_projection The matrix to write the projection matrix to.
    void view_projection_perspective_camera(const perspective_camera& camera, const vec3& camera_position, mat4& out_view, mat4& out_projection);

    //! \brief Creates view and projection matrix from a given \a orthographic_camera.
    //! \param[in] camera The \a orthographic_camera to create the matrices from.
    //! \param[in] camera_position The position of the camera.
    //! \param[out] out_view The matrix to write the view matrix to.
    //! \param[out] out_projection The matrix to write the projection matrix to.
    void view_projection_orthographic_camera(const orthographic_camera& camera, const vec3& camera_position, mat4& out_view, mat4& out_projection);
} // namespace mango

#endif // MANGO_SCENE_HELPER_HPP