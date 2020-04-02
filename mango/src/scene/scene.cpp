//! \file      scene.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <mango/scene.hpp>
#include <mango/scene_types.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/geometry_objects.hpp>
#include <resources/resource_system.hpp>

using namespace mango;

static void scene_graph_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void transformation_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void camera_update(scene_component_manager<camera_component>& cameras, scene_component_manager<transform_component>& transformations);
static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_manager<mesh_component>& meshes, scene_component_manager<transform_component>& transformations);

scene::scene(const string& name)
{
    MANGO_UNUSED(name);
}

scene::~scene() {}

entity scene::create_empty()
{
    static uint32 id = 1;                                                   // TODO Paul: This should be done in a better way!
    MANGO_ASSERT(id < max_entities, "Reached maximum number of entities!"); // TODO Paul: This assertion is false, because of deleted ones there could be places left.
    entity new_entity = id++;
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
    std::vector<entity> scene_entities;
    entity scene_root = create_empty();
    scene_entities.push_back(scene_root);
    shared_ptr<resource_system> rs = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(rs, "Resource System is invalid!");
    model_configuration config     = { "gltf_model_" + scene_root };
    const shared_ptr<model> loaded = rs->load_gltf(path, config);
    tinygltf::Model& m             = loaded->gltf_model;

    // load the default scene or the first one.
    MANGO_ASSERT(m.scenes.size() > 0, "No scenes in the gltf model found!");
    int scene_id                 = m.defaultScene > -1 ? m.defaultScene : 0;
    const tinygltf::Scene& scene = m.scenes[scene_id];
    for (uint32 i = 0; i < scene.nodes.size(); ++i)
    {
        entity node = build_model_node(scene_entities, m, m.nodes.at(scene.nodes.at(i)));

        attach(node, scene_root);
    }

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

    render_meshes(rs, m_meshes, m_transformations);
}

void scene::attach(entity child, entity parent)
{
    if (m_nodes.contains(child))
    {
        detach(child);
    }

    m_nodes.create_component_for(child).parent_entity = parent;

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

    transform_component* child_transform = m_transformations.get_component_for_entity(child);
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

entity scene::build_model_node(std::vector<entity>& entities, tinygltf::Model& m, tinygltf::Node& n)
{
    entity node     = create_empty();
    auto& transform = m_transformations.create_component_for(node);
    if (n.matrix.size() == 16)
    {
        // matrix
        transform.local_transformation_matrix = glm::mat4(*(float*)n.matrix.data());
    }
    else
    {
        // Translation x Rotation x Scale
        if (n.scale.size() == 3)
        {
            transform.local_transformation_matrix = glm::scale(transform.local_transformation_matrix, glm::vec3(n.scale[0], n.scale[1], n.scale[2]));
        }

        if (n.rotation.size() == 4)
        {
            transform.local_transformation_matrix = glm::rotate(transform.local_transformation_matrix, (float)n.rotation[0], glm::vec3(n.rotation[1], n.rotation[2], n.rotation[3]));
        }

        if (n.translation.size() == 3)
        {
            transform.local_transformation_matrix = glm::translate(transform.local_transformation_matrix, glm::vec3(n.translation[0], n.translation[1], n.translation[2]));
        }
    }
    if (n.mesh > -1)
    {
        MANGO_ASSERT((uint32)n.mesh < m.meshes.size(), "Invalid gltf mesh!");
        build_model_mesh(node, m, m.meshes.at(n.mesh));
    }

    entities.push_back(node);

    // build child nodes.
    for (uint32 i = 0; i < n.children.size(); ++i)
    {
        MANGO_ASSERT((uint32)n.children[i] < m.nodes.size(), "Invalid gltf node!");

        entity child = build_model_node(entities, m, m.nodes.at(n.children.at(i)));
        attach(child, node);
    }

    return node;
}

void scene::build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh)
{
    std::map<uint32, uint32> vbos;
    auto& component_mesh                      = m_meshes.create_component_for(node);
    component_mesh.vertex_array_object_handle = create_empty_vertex_array_object();
    glBindVertexArray(component_mesh.vertex_array_object_handle);

    for (size_t i = 0; i < m.bufferViews.size(); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_WARN("Buffer view target is zero!");
            continue;
        }

        const tinygltf::Buffer& buffer = m.buffers[buffer_view.buffer];

        GLuint vbo;
        glGenBuffers(1, &vbo);
        vbos.insert({ i, vbo });
        glBindBuffer(buffer_view.target, vbo);

        glBufferData(buffer_view.target, buffer_view.byteLength, &buffer.data.at(0) + buffer_view.byteOffset, GL_STATIC_DRAW);
    }

    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        tinygltf::Primitive primitive = mesh.primitives[i];

        if (primitive.indices < 0)
        {
            MANGO_LOG_DEBUG("No primitives in this gltf mesh!");
            return;
        }

        tinygltf::Accessor index_accessor = m.accessors[primitive.indices];

        primitive_component p;
        auto& s = p.draw_data.state; // TODO Paul: State may not be correct.
        s.changed = true;
        s.blending = blend_off;
        s.cull = cull_backface;
        s.depth = depth_less;
        s.wireframe = wireframe_off;
        p.draw_data.gpu_call = draw_elements;
        p.draw_data.gpu_primitive = (gpu_primitive_type)primitive.mode;
        p.draw_data.count = index_accessor.count;
        p.draw_data.component_type = index_accessor.componentType;
        p.draw_data.byte_offset = index_accessor.byteOffset;
        component_mesh.primitives.push_back(p);

        component_mesh.primitives.back().primitive_draw_call = { draw_call , &component_mesh.primitives.back().draw_data };



        for (auto& attrib : primitive.attributes)
        {
            tinygltf::Accessor accessor = m.accessors[attrib.second];
            int byte_stride             = accessor.ByteStride(m.bufferViews[accessor.bufferView]);
            glBindBuffer(GL_ARRAY_BUFFER, vbos.at(accessor.bufferView));

            int size = 1;
            if (accessor.type != TINYGLTF_TYPE_SCALAR)
            {
                size = accessor.type;
            }

            int attrib_array = -1;
            if (attrib.first.compare("POSITION") == 0)
                attrib_array = 0;
            if (attrib.first.compare("NORMAL") == 0)
                attrib_array = 1;
            if (attrib_array > -1)
            {
                glEnableVertexAttribArray(attrib_array);
                glVertexAttribPointer(attrib_array, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, byte_stride, (char*)NULL + accessor.byteOffset);
            }
            else
                MANGO_LOG_DEBUG("Vertex attribute array is missing: {0}!", attrib.first);
        }
    }

    glBindVertexArray(0);
    // TODO Paul: Check, why deleting is not possible here.
    // for (uint32 i = 0; i < vbos.size(); ++i)
    //     glDeleteBuffers(1, &(vbos.at(i)));

    component_mesh.mesh_binding_data.handle = component_mesh.vertex_array_object_handle;
    component_mesh.mesh_binding_command = { vao_binding, &component_mesh.mesh_binding_data };
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
            entity e                       = cameras.entity_at(index);
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

static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_manager<mesh_component>& meshes, scene_component_manager<transform_component>& transformations)
{
    meshes.for_each(
        [&rs, &meshes, &transformations](mesh_component& c, uint32& index) {
            entity e                       = meshes.entity_at(index);
            transform_component* transform = transformations.get_component_for_entity(e);
            if (transform)
            {
                rs->submit(c.mesh_binding_command);
                for(auto& p: c.primitives)
                {
                    rs->submit(p.primitive_draw_call);
                }
            }
        },
        false);
}