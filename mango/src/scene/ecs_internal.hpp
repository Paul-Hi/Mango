//! \file      ecs_internal.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_ECS_INTERNAL_HPP
#define MANGO_ECS_INTERNAL_HPP

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <mango/profile.hpp>
#include <mango/scene_component_pool.hpp>
#include <mango/scene_ecs.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief An \a ecsystem for transformation updates.
    class transformation_update_system : public ecsystem_1<transform_component>
    {
      public:
        void execute(float, scene_component_pool<transform_component>& transformations) override
        {
            NAMED_PROFILE_ZONE("Transformation Update");
            transformations.for_each(
                [&transformations](transform_component& c, int32&) {
                    c.local_transformation_matrix = glm::translate(glm::mat4(1.0), c.position);
                    c.local_transformation_matrix = c.local_transformation_matrix * glm::toMat4(c.rotation);
                    c.local_transformation_matrix = glm::scale(c.local_transformation_matrix, c.scale);

                    c.world_transformation_matrix = c.local_transformation_matrix;
                },
                false);
        }
    };

    //! \brief An \a ecsystem for scene graph updates.
    class scene_graph_update_system : public ecsystem_2<node_component, transform_component>
    {
      public:
        void execute(float, scene_component_pool<node_component>& nodes, scene_component_pool<transform_component>& transformations) override
        {
            NAMED_PROFILE_ZONE("Scene Graph Update");
            nodes.for_each(
                [&nodes, &transformations](node_component& c, int32& index) {
                    node_component& parent_component = c;
                    if (c.parent_entity == invalid_entity)
                        return;
                    entity e = nodes.entity_at(index);

                    transform_component* child_transform  = transformations.get_component_for_entity(e);
                    transform_component* parent_transform = transformations.get_component_for_entity(parent_component.parent_entity);
                    if (nullptr != child_transform && nullptr != parent_transform)
                    {
                        child_transform->world_transformation_matrix = parent_transform->world_transformation_matrix * child_transform->local_transformation_matrix;
                    }
                },
                false);
        }
    };

    //! \brief An \a ecsystem for camera updates.
    class camera_update_system : public ecsystem_2<camera_component, transform_component>
    {
      public:
        void execute(float, scene_component_pool<camera_component>& cameras, scene_component_pool<transform_component>& transformations) override
        {
            NAMED_PROFILE_ZONE("Camera Update");
            cameras.for_each(
                [&cameras, &transformations](camera_component& c, int32& index) {
                    entity e                       = cameras.entity_at(index);
                    transform_component* transform = transformations.get_component_for_entity(e);
                    if (transform)
                    {
                        glm::vec3 front = c.target - glm::vec3(transform->world_transformation_matrix[3]);
                        if (glm::length(front) > 1e-5)
                            front = glm::normalize(front);
                        else
                        {
                            front    = GLOBAL_FORWARD;
                            c.target = front * 0.1f; // We need this here.
                        }
                        auto right = glm::normalize(glm::cross(GLOBAL_UP, front));
                        c.up       = glm::normalize(glm::cross(front, right));
                        c.view     = glm::lookAt(glm::vec3(transform->world_transformation_matrix[3]), c.target, c.up);
                        if (c.cam_type == camera_type::perspective_camera)
                        {
                            c.projection = glm::perspective(c.perspective.vertical_field_of_view, c.perspective.aspect, c.z_near, c.z_far);
                        }
                        else if (c.cam_type == camera_type::orthographic_camera)
                        {
                            const float distance_x = c.orthographic.x_mag;
                            const float distance_y = c.orthographic.y_mag;
                            c.projection           = glm::ortho(-distance_x * 0.5f, distance_x * 0.5f, -distance_y * 0.5f, distance_y * 0.5f, c.z_near, c.z_far);
                        }
                        c.view_projection = c.projection * c.view;
                    }
                },
                false);
        }
    };

    //! \brief An \a ecsystem for rendering meshes.
    class render_mesh_system : public ecsystem_2<mesh_component, transform_component>
    {
      public:
        //! \brief Setup for the \a render_mesh_system. Needs to be called before executing.
        //! \param[in] rs The \a render_system to submit the meshes to.
        void setup(shared_ptr<render_system_impl> rs)
        {
            m_rs = rs;
        }

        void execute(float, scene_component_pool<mesh_component>& meshes, scene_component_pool<transform_component>& transformations) override
        {
            PROFILE_ZONE;
            meshes.for_each(
                [this, &meshes, &transformations](mesh_component& c, int32& index) {
                    entity e                       = meshes.entity_at(index);
                    transform_component* transform = transformations.get_component_for_entity(e);
                    if (transform)
                    {
                        m_rs->begin_mesh(transform->world_transformation_matrix, c.has_normals, c.has_tangents);

                        for (int32 i = 0; i < static_cast<int32>(c.primitives.size()); ++i)
                        {
                            auto m = c.materials[i];
                            auto p = c.primitives[i];
                            m_rs->use_material(m.component_material);
                            m_rs->draw_mesh(p.vertex_array_object, p.topology, p.first, p.count, p.type_index, p.instance_count);
                        }
                        m_rs->end_mesh();
                    }
                },
                false);
        }

      private:
        //! \brief The \a render_system to submit the meshes to.
        shared_ptr<render_system_impl> m_rs;
    };

    //! \brief An \a ecsystem for light submission.
    class light_submission_system : public ecsystem_1<light_component>
    {
      public:
        //! \brief Setup for the \a light_submission_system. Needs to be called before executing.
        //! \param[in] rs The \a render_system to submit the lights to.
        void setup(shared_ptr<render_system_impl> rs)
        {
            m_rs = rs;
        }

        void execute(float, scene_component_pool<light_component>& lights) override
        {
            PROFILE_ZONE;
            lights.for_each(
                [this, &lights](light_component& c, int32&) {
                    m_rs->submit_light(c.type_of_light, c.data.get());
                },
                false);
        }

      private:
        //! \brief The \a render_system to submit the lights to.
        shared_ptr<render_system_impl> m_rs;
    };

} // namespace mango

#endif // MANGO_ECS_INTERNAL_HPP
