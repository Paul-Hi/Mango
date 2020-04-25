//! \file      scene.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <graphics/buffer.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_program.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/scene.hpp>
#include <mango/scene_types.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>

using namespace mango;

//! \brief All data that is also used as an uniform. Put everything in there that is needed.
struct scene_uniforms : public uniform_data
{
    glm::mat4 model_matrix; //!< The transformation matrix.
    glm::mat4 view_projection; //!< The cameras view projection matrix.
};

static void scene_graph_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void transformation_update(scene_component_manager<node_component>& nodes, scene_component_manager<transform_component>& transformations);
static void camera_update(scene_component_manager<camera_component>& cameras, scene_component_manager<transform_component>& transformations);
static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_manager<mesh_component>& meshes, scene_component_manager<transform_component>& transformations, scene_uniforms uniforms);

scene::scene(const string& name)
    : m_nodes()
    , m_transformations()
    , m_meshes()
    , m_cameras()
{
    MANGO_UNUSED(name);

    shader_configuration v_shader_config;
    v_shader_config.m_path = "res/shader/v_hello_gltf.glsl";
    v_shader_config.m_type = shader_type::VERTEX_SHADER;

    shader_ptr hello_vertex = shader::create(v_shader_config);

    shader_configuration f_shader_config;
    f_shader_config.m_path = "res/shader/f_hello_gltf.glsl";
    f_shader_config.m_type = shader_type::FRAGMENT_SHADER;

    shader_ptr hello_fragment = shader::create(f_shader_config);

    m_scene_shader_program = shader_program::create_graphics_pipeline(hello_vertex, nullptr, nullptr, nullptr, hello_fragment);
}

scene::~scene() {}

entity scene::create_empty()
{
    static uint32 id = 1;                                                   // TODO Paul: This should be done in a better way!
    MANGO_ASSERT(id < max_entities, "Reached maximum number of entities!"); // TODO Paul: This assertion is false, because of deleted ones there could be places left.
    entity new_entity = id;
    id++;
    return new_entity;
}

entity scene::create_default_camera()
{
    entity camera_entity      = create_empty();
    auto& camera_component    = m_cameras.create_component_for(camera_entity);
    auto& transform_component = m_transformations.create_component_for(camera_entity);

    // default parameters
    camera_component.type                   = camera_type::perspective_camera;
    camera_component.aspect                 = 16.0f / 9.0f;
    camera_component.z_near                 = 0.1f;
    camera_component.z_far                  = 100.0f;
    camera_component.vertical_field_of_view = glm::radians(45.0f);

    transform_component.local_transformation_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform_component.world_transformation_matrix = transform_component.local_transformation_matrix;

    // const float distance = camera_component.z_far - camera_component.z_near
    camera_component.view_projection =
        glm::perspective(camera_component.vertical_field_of_view, camera_component.aspect, camera_component.z_near, camera_component.z_far) * transform_component.world_transformation_matrix;
    // glm::ortho(-camera_component.aspect * distance, camera_component.aspect * distance, -distance, distance);

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

    return scene_entities;
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

    scene_uniforms uniforms;

    // We have at least one default camera in each scene and at the moment the first camera is the active one everytime.
    if (m_cameras.size() == 0)
        create_default_camera();
    uniforms.view_projection = m_cameras.component_at(0).view_projection;

    auto cmdb = rs->get_command_buffer();
    cmdb->bind_shader_program(m_scene_shader_program);
    render_meshes(rs, m_meshes, m_transformations, uniforms);
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
    auto& component_mesh               = m_meshes.create_component_for(node);
    component_mesh.vertex_array_object = vertex_array::create();

    // Bind vertex buffers later, so we do not bind not used ones and can determine tightly packed buffer stride.
    struct vb_data
    {
        buffer_ptr vb;
        ptr_size stride;
    };
    std::map<int, vb_data> index_to_vb_data;

    for (size_t i = 0; i < m.bufferViews.size(); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_WARN("Buffer view target is zero!");
            continue;
        }

        const tinygltf::Buffer& t_buffer = m.buffers[buffer_view.buffer];

        buffer_configuration config;
        config.m_access       = buffer_access::MAPPED_ACCESS_WRITE;
        config.m_size         = buffer_view.byteLength;
        config.m_target       = buffer_view.target == GL_ARRAY_BUFFER ? buffer_target::VERTEX_BUFFER : buffer_target::INDEX_BUFFER;
        buffer_ptr buf        = buffer::create(config);
        unsigned char* mapped = static_cast<unsigned char*>(buf->map(0, buffer_view.byteLength, buffer_access::MAPPED_ACCESS_WRITE));
        memcpy(mapped, const_cast<unsigned char*>(t_buffer.data.data() + buffer_view.byteOffset), buffer_view.byteLength);

        if (buffer_view.target == GL_ELEMENT_ARRAY_BUFFER)
        {
            component_mesh.vertex_array_object->bind_index_buffer(buf);
        }
        else
        {
            vb_data data = { buf, buffer_view.byteStride };
            index_to_vb_data.insert({ i, data });
        }
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
        p.count          = index_accessor.count;
        p.topology       = static_cast<primitive_topology>(primitive.mode); // cast is okay.
        p.first          = index_accessor.byteOffset;
        p.type_index     = static_cast<index_type>(index_accessor.componentType);
        p.instance_count = 1;

        component_mesh.primitives.push_back(p);

        uint32 vb_idx = 0;
        std::map<int, int> index_to_binding_point;

        for (auto& attrib : primitive.attributes)
        {
            tinygltf::Accessor accessor = m.accessors[attrib.second];

            format attribute_format = get_attribute_format(static_cast<format>(accessor.componentType), accessor.type % 32); // TODO Paul: Type is not handled properly here.

            int attrib_array = -1;
            if (attrib.first.compare("POSITION") == 0)
                attrib_array = 0;
            if (attrib.first.compare("NORMAL") == 0)
                attrib_array = 1;
            if (attrib_array > -1)
            {
                auto it = index_to_vb_data.find(accessor.bufferView);
                if (it != index_to_vb_data.end())
                {
                    ptr_size stride = it->second.stride != 0 ? it->second.stride : accessor.ByteStride(m.bufferViews[accessor.bufferView]);
                    component_mesh.vertex_array_object->bind_vertex_buffer(vb_idx, it->second.vb, 0, stride);
                    index_to_vb_data.erase(accessor.bufferView); // Remove because we bound it.
                    index_to_binding_point.insert({ accessor.bufferView, vb_idx });
                    vb_idx++;
                }

                component_mesh.vertex_array_object->set_vertex_attribute(attrib_array, index_to_binding_point.at(accessor.bufferView), attribute_format, accessor.byteOffset);
            }
            else
                MANGO_LOG_DEBUG("Vertex attribute array is missing: {0}!", attrib.first);
        }
    }
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
                if (c.type == camera_type::perspective_camera)
                    c.view_projection = glm::perspective(c.vertical_field_of_view, c.aspect, c.z_near, c.z_far) * transform->world_transformation_matrix;
                else if (c.type == camera_type::orthographic_camera)
                {
                    const float distance = c.z_far - c.z_near;
                    c.view_projection    = glm::ortho(-c.aspect * distance, c.aspect * distance, -distance, distance) * transform->world_transformation_matrix;
                }
            }
        },
        false);
}

static void render_meshes(shared_ptr<render_system_impl> rs, scene_component_manager<mesh_component>& meshes, scene_component_manager<transform_component>& transformations, scene_uniforms uniforms)
{
    meshes.for_each(
        [&rs, &meshes, &transformations, &uniforms](mesh_component& c, uint32& index) {
            entity e                       = meshes.entity_at(index);
            transform_component* transform = transformations.get_component_for_entity(e);
            if (transform)
            {
                uniforms.model_matrix = transform->world_transformation_matrix;
                auto cmdb             = rs->get_command_buffer();
                cmdb->bind_vertex_array(c.vertex_array_object);
                rs->get_command_buffer()->bind_single_uniforms(&uniforms, sizeof(scene_uniforms));
                for (auto& p : c.primitives)
                {
                    cmdb->draw_elements(p.topology, p.first, p.count, p.type_index, p.instance_count);
                }
            }
        },
        false);
}