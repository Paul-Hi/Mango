//! \file      scene.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <mango/scene.hpp>
#include <mango/scene_types.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

static void scene_graph_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void transformation_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void camera_update(scene_component_manager<camera_component>& cameras, scene_component_manager<transform_component>& transformations);

scene::scene(const string& name)
{
    MANGO_UNUSED(name);
}

scene::~scene() {}

entity scene::create_empty()
{
    static uint32 id = 1;                                                   // TODO Paul: This should be done in a better way!
    MANGO_ASSERT(id < max_entities, "Reached maximum number of entities!"); // TODO Paul: This assertion is false, because of deleted ones there could be places left.
    entity new_entity = id;
    return new_entity;
}

entity scene::create_default_camera()
{
    entity camera_entity      = create_empty();
    auto& camera_component    = m_cameras.create_component_for(camera_entity);
    auto& transform_component = m_transformations.create_component_for(camera_entity);

    // default parameters
    camera_component.type                   = perspective_camera;
    camera_component.aspect                 = 16.0f / 9.0f;
    camera_component.z_near                 = 0.1f;
    camera_component.z_far                  = 100.0f;
    camera_component.vertical_field_of_view = glm::radians(45.0f);

    transform_component.local_transformation_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform_component.world_transformation_matrix = transform_component.local_transformation_matrix;

    // const float distance = camera_component.z_far - camera_component.z_near;
    camera_component.view_projection =
        glm::perspective(camera_component.vertical_field_of_view, camera_component.aspect, camera_component.z_near, camera_component.z_far) * transform_component.world_transformation_matrix;
    // glm::ortho(-camera_component.aspect * distance, camera_component.aspect * distance, -distance, distance);

    camera_component.camera_uniform_data   = { "u_view_projection_matrix", &(camera_component.view_projection[0]) };
    camera_component.camera_render_command = { uniform_binding, &camera_component.camera_uniform_data };

    return camera_entity;
}

std::vector<entity> scene::create_entities_from_model(const string& path)
{
    MANGO_UNUSED(path);
    return { invalid_entity };
}

void scene::update(float dt)
{
    MANGO_UNUSED(dt);
    scene_graph_update(m_nodes, m_transformations);
    transformation_update(m_nodes, m_transformations);
    camera_update(m_cameras, m_transformations);
}

void scene::render()
{
    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");

    // We have at least one default camera in each scene and at the moment the first camera is the active one everytime.
    if (m_cameras.size() == 0)
        create_default_camera();
    rs->submit(m_cameras.component_at(0).camera_render_command);
}

void scene::attach(entity child, entity parent)
{
    if (m_nodes.contains(child))
    {
        detach(child);
    }

    auto c = m_nodes.create_component_for(child).parent_entity = parent;
    MANGO_UNUSED(c);

    // reorder subtrees if necessary
    if (m_nodes.size() > 1)
    {
        m_nodes.for_each(
            [this](node_component, uint32& index) {
                entity possible_parent = m_nodes.entity_at(index);
                for (size_t j = 0; j < index; ++j)
                {
                    const node_component& possible_child = m_nodes.component_at(j);

                    if (possible_child.parent_entity == possible_parent)
                    {
                        m_nodes.move(index, j);
                        ++index; // TODO Paul: Test if this is correct.
                        break;
                    }
                }
            },
            true);
    }

    node_component& parent_component = *m_nodes.get_component_for_entity(child);

    transform_component* parent_transform = m_transformations.get_component_for_entity(parent);
    if (nullptr == parent_transform)
    {
        // create transform component for parent if non-existent
        parent_transform = &m_transformations.create_component_for(parent);
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(parent);
    if (nullptr == child_transform)
    {
        // create transform component for child if non-existent
        child_transform = &m_transformations.create_component_for(child);
    }
    parent_component.parent_transformation_matrix = parent_transform->world_transformation_matrix;
}

void scene::detach(entity child)
{
    node_component* parent_component = m_nodes.get_component_for_entity(child);

    if (nullptr == parent_component)
    {
        MANGO_LOG_DEBUG("Entity has no parent!");
        return;
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(child);

    if (nullptr != child_transform)
    {
        // Add transformation from parent before removing the node hierarchy
        child_transform->local_transformation_matrix = child_transform->world_transformation_matrix;
    }

    // We want to remove it without crashing the order. In that way, we don't need to sort it again.
    m_nodes.sort_remove_component_from(child);
}

static void scene_graph_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations)
{
    nodes.for_each(
        [&nodes, &transformations](node_component& c, uint32& index) {
            const node_component& parent_component = c;
            entity e                               = nodes.entity_at(index);

            transform_component* child_transform  = transformations.get_component_for_entity(e);
            transform_component* parent_transform = transformations.get_component_for_entity(parent_component.parent_entity);
            if (nullptr != child_transform && nullptr != parent_transform)
            {
                child_transform->world_transformation_matrix = child_transform->local_transformation_matrix * parent_transform->world_transformation_matrix;
            }
        },
        false);
}

static void transformation_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations)
{
    transformations.for_each(
        [&nodes, &transformations](transform_component& c, uint32& index) {
            entity e       = transformations.entity_at(index);
            bool no_update = nodes.contains(e);
            if (!no_update)
            {
                c.world_transformation_matrix = c.local_transformation_matrix;
            }
        },
        false);
}

static void camera_update(scene_component_manager<camera_component>& cameras, scene_component_manager<transform_component>& transformations)
{
    cameras.for_each(
        [&cameras, &transformations](camera_component& c, uint32& index) {
            entity e                       = transformations.entity_at(index);
            transform_component* transform = transformations.get_component_for_entity(e);
            if (transform)
            {
                if (c.type == perspective_camera)
                    c.view_projection = glm::perspective(c.vertical_field_of_view, c.aspect, c.z_near, c.z_far) * transform->world_transformation_matrix;
                else if (c.type == orthographic_camera)
                {
                    const float distance = c.z_far - c.z_near;
                    c.view_projection    = glm::ortho(-c.aspect * distance, c.aspect * distance, -distance, distance) * transform->world_transformation_matrix;
                }
            }
        },
        false);
}
