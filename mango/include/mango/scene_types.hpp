//! \file      scene_types.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_SCENE_TYPES_HPP
#define MANGO_SCENE_TYPES_HPP

#include <glm/glm.hpp>
#include <mango/render_structures.hpp>

namespace mango
{
    //! \brief An \a entity. Just a integer used as an id.
    using entity                = uint32;
    //! \brief Invalid \a entity.
    const entity invalid_entity = 0;
    //! \brief Maximum number of \a entities in mango.
    const entity max_entities   = 1000; // TODO Paul: Extend if necessary.

    //! \brief Component used to transform anything in the scene.
    struct transform_component
    {
        glm::mat4 world_transformation_matrix = glm::mat4(1.0f); //!< The world transformation.
        glm::mat4 local_transformation_matrix = glm::mat4(1.0f); //!< The local transformation. If there is no parent this is also the world transformation.
    };

    //! \brief Component used to build a graph like structure. This is necessary for parenting.
    struct node_component
    {
        entity parent_entity = invalid_entity; //!< The parents entity id.
        glm::mat4 parent_transformation_matrix; //!< The parents world transformation.
    };

    //! \brief Camera types used in \a camera_components.
    enum camera_type
    {
        perspective_camera, //!< Perspective projection. Usually usefull for 3D scenes.
        orthographic_camera //!< Orthographic projection. Usually usefull for 2D scenes or UI.
    };

    //! \brief Component used for camera entities.
    struct camera_component
    {
        //! \brief The view projection matrix of the \a camera_component.
        glm::mat4 view_projection;

        camera_type type; //!< The type of camera projection.

        float z_near;                 //!< Distance of the near plane.
        float z_far;                  //!< Distance of the far plane.
        float vertical_field_of_view; //!< Vertical field of view in radians.
        float aspect;                 //!< Aspect ratio. Width divided by height.

        uniform_binding_data camera_uniform_data; //!< Uniform data used for rendering.
        render_command camera_render_command; //!< Render command.
    };
} // namespace mango

#endif // MANGO_SCENE_TYPES_HPP