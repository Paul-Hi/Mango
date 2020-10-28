//! \file      scene.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <core/context_impl.hpp>
#include <glad/glad.h>
#include <glm/gtx/quaternion.hpp>
#include <graphics/buffer.hpp>
#include <graphics/texture.hpp>
#include <graphics/vertex_array.hpp>
#include <mango/scene.hpp>
#include <rendering/render_system_impl.hpp>
#include <resources/resource_system.hpp>
#include <scene/ecs_internal.hpp>

using namespace mango;

//! \brief The internal \a ecsystem for transformation updates.
transformation_update_system transformation_update;
//! \brief The internal \a ecsystem for scene graph updates.
scene_graph_update_system scene_graph_update;
//! \brief The internal \a ecsystem for camera updates.
camera_update_system camera_update;
//! \brief The internal \a ecsystem for mesh rendering.
render_mesh_system render_mesh;
//! \brief The internal \a ecsystem for submitting lights.
light_submission_system light_submission;

static void update_scene_boundaries(glm::mat4& trafo, tinygltf::Model& m, tinygltf::Mesh& mesh, glm::vec3& min, glm::vec3& max);

scene::scene(const string& name)
    : m_nodes()
    , m_transformations()
    , m_meshes()
    , m_cameras()
    , m_environments()
    , m_lights()
{
    PROFILE_ZONE;
    MANGO_UNUSED(name);
    m_active.camera        = invalid_entity;
    m_active.environment   = invalid_entity;
    m_scene_boundaries.max = glm::vec3(-3.402823e+38f);
    m_scene_boundaries.min = glm::vec3(3.402823e+38f);

    for (uint32 i = 1; i <= max_entities; ++i)
        m_free_entities.push_back(i);

    m_root_entity          = create_empty();
    auto& tag_component    = m_tags.create_component_for(m_root_entity);
    tag_component.tag_name = "Scene Root";
}

scene::~scene() {}

entity scene::create_empty()
{
    PROFILE_ZONE;
    MANGO_ASSERT(!m_free_entities.empty(), "Reached maximum number of entities!");
    entity new_entity = m_free_entities.front();
    m_free_entities.pop_front();
    MANGO_LOG_DEBUG("Created entity {0}, {1} left", new_entity, m_free_entities.size());
    return new_entity;
}

void scene::remove_entity(entity e)
{
    PROFILE_ZONE;
    if (e == invalid_entity || !is_entity_alive(e))
        return;

    auto children = get_children(e);
    for (auto child : children)
    {
        remove_entity(child);
    }
    delete_node(e);
    m_transformations.remove_component_from(e);
    m_meshes.remove_component_from(e);
    bool del_active = get_active_camera_data().active_camera_entity == e;
    m_cameras.remove_component_from(e);
    if (del_active)
    {
        if (m_cameras.size() > 0)
            set_active_camera(m_cameras.entity_at(0));
        else
            set_active_camera(invalid_entity);
    }
    del_active = get_active_environment_data().active_environment_entity == e;
    m_environments.remove_component_from(e);
    if (del_active)
    {
        if (m_environments.size() > 0)
            set_active_environment(m_environments.entity_at(0));
        else
            set_active_environment(invalid_entity);
    }
    m_free_entities.push_back(e);
    MANGO_LOG_DEBUG("Removed entity {0}, {1} left", e, m_free_entities.size());
}

entity scene::create_default_camera()
{
    PROFILE_ZONE;
    entity camera_entity = create_empty();
    attach(camera_entity, m_root_entity);
    auto& camera_component    = m_cameras.create_component_for(camera_entity);
    auto& transform_component = m_transformations.create_component_for(camera_entity);
    auto& tag_component       = m_tags.create_component_for(camera_entity);
    tag_component.tag_name    = "Default Camera";

    // default parameters
    camera_component.cam_type                           = camera_type::perspective_camera;
    camera_component.perspective.aspect                 = 16.0f / 9.0f;
    camera_component.z_near                             = 0.04f;
    camera_component.z_far                              = 40.0f;
    camera_component.perspective.vertical_field_of_view = glm::radians(45.0f);
    camera_component.up                                 = glm::vec3(0.0f, 1.0f, 0.0f);
    camera_component.target                             = glm::vec3(0.0f, 0.0f, 0.0f);
    camera_component.physical.aperture                  = 16.0f;
    camera_component.physical.shutter_speed             = 1.0f / 125.0f;
    camera_component.physical.iso                       = 100.0f;

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 1.5f);

    transform_component.position = position;
    set_active_camera(camera_entity);

    return camera_entity;
}

entity scene::create_entities_from_model(const string& path, entity gltf_root)
{
    PROFILE_ZONE;
    if (gltf_root == invalid_entity)
    {
        gltf_root = create_empty();
        attach(gltf_root, m_root_entity);
    }

    auto& model_comp               = m_models.create_component_for(gltf_root);
    model_comp.model_file_path     = path;
    shared_ptr<resource_system> rs = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(rs, "Resource System is expired!");
    auto start                                      = path.find_last_of("\\/") + 1;
    auto name                                       = path.substr(start, path.find_last_of(".") - start);
    m_tags.create_component_for(gltf_root).tag_name = path.substr(start);
    model_configuration config                      = { name };
    const shared_ptr<model> loaded                  = rs->get_gltf_model(path, config);
    if (!loaded)
        return invalid_entity;

    tinygltf::Model& m = loaded->gltf_model;

    // load the default scene or the first one.
    glm::vec3 max_backup   = m_scene_boundaries.max;
    glm::vec3 min_backup   = m_scene_boundaries.min;
    m_scene_boundaries.max = glm::vec3(-3.402823e+38f);
    m_scene_boundaries.min = glm::vec3(3.402823e+38f);
    if (m.scenes.size() <= 0)
    {
        MANGO_LOG_DEBUG("No scenes in the gltf model found! Can not load invalid gltf.");
        return invalid_entity;
    }

    // load all model buffer views into buffers.
    std::map<int, buffer_ptr> index_to_buffer_data;

    for (int32 i = 0; i < static_cast<int32>(m.bufferViews.size()); ++i)
    {
        const tinygltf::BufferView& buffer_view = m.bufferViews[i];
        if (buffer_view.target == 0)
        {
            MANGO_LOG_WARN("Buffer view target is zero!"); // We can continue here.
        }

        const tinygltf::Buffer& t_buffer = m.buffers[buffer_view.buffer];

        buffer_configuration buffer_config;
        buffer_config.m_access = buffer_access::none;
        buffer_config.m_size   = buffer_view.byteLength;
        buffer_config.m_target = (buffer_view.target == 0 || buffer_view.target == GL_ARRAY_BUFFER) ? buffer_target::vertex_buffer : buffer_target::index_buffer;

        const unsigned char* buffer_start = t_buffer.data.data() + buffer_view.byteOffset;
        const void* buffer_data           = static_cast<const void*>(buffer_start);
        buffer_config.m_data              = buffer_data;
        buffer_ptr buf                    = buffer::create(buffer_config);
        // TODO Paul: Interleaved buffers could be loaded two times ... BAD.

        index_to_buffer_data.insert({ i, buf });
    }

    int scene_id                 = m.defaultScene > -1 ? m.defaultScene : 0;
    const tinygltf::Scene& scene = m.scenes[scene_id];
    for (int32 i = 0; i < static_cast<int32>(scene.nodes.size()); ++i)
    {
        entity node = build_model_node(m, m.nodes.at(scene.nodes.at(i)), glm::mat4(1.0), index_to_buffer_data);

        attach(node, gltf_root);
    }

    // normalize scale
    const glm::vec3 scale                                        = glm::vec3(10.0f / (glm::compMax(m_scene_boundaries.max - m_scene_boundaries.min)));
    m_transformations.get_component_for_entity(gltf_root)->scale = scale;
    model_comp.min_extends                                       = m_scene_boundaries.min;
    model_comp.max_extends                                       = m_scene_boundaries.max;

    if (m_active.camera == invalid_entity)
    {
        create_default_camera();
    }

    m_cameras.get_component_for_entity(m_active.camera)->target = (m_scene_boundaries.max + m_scene_boundaries.min) * 0.5f * scale;

    m_scene_boundaries.max =
        glm::max(m_scene_boundaries.max, max_backup); // TODO Paul: This is just in case all other assets are still here, we need to do the calculation with all still existing entities.
    m_scene_boundaries.min =
        glm::min(m_scene_boundaries.min, min_backup); // TODO Paul: This is just in case all other assets are still here, we need to do the calculation with all still existing entities.


    return gltf_root;
}

entity scene::create_environment_from_hdr(const string& path)
{
    PROFILE_ZONE;
    entity environment_entity = create_empty();
    attach(environment_entity, m_root_entity);
    auto& environment = m_environments.create_component_for(environment_entity);

    // default rotation and scale
    environment.rotation_scale_matrix = glm::mat3(1.0f);

    // load image and texture
    shared_ptr<resource_system> res = m_shared_context->get_resource_system_internal().lock();
    MANGO_ASSERT(res, "Resource System is expired!");

    image_configuration img_config;
    auto start                                               = path.find_last_of("\\/") + 1;
    img_config.name                                          = path.substr(start, path.find_last_of(".") - start);
    img_config.is_standard_color_space                       = false;
    img_config.is_hdr                                        = true;
    m_tags.create_component_for(environment_entity).tag_name = path.substr(start);

    auto hdr_image = res->get_image(path, img_config);

    texture_configuration tex_config;
    tex_config.m_generate_mipmaps        = 1;
    tex_config.m_is_standard_color_space = false;
    tex_config.m_texture_min_filter      = texture_parameter::filter_linear;
    tex_config.m_texture_mag_filter      = texture_parameter::filter_linear;
    tex_config.m_texture_wrap_s          = texture_parameter::wrap_clamp_to_edge;
    tex_config.m_texture_wrap_t          = texture_parameter::wrap_clamp_to_edge;

    texture_ptr hdr_texture = texture::create(tex_config);

    format f        = format::rgb;
    format internal = format::rgb32f;
    format type     = format::t_float;

    if (hdr_image->number_components == 4)
    {
        f        = format::rgba;
        internal = format::rgba32f;
    }

    hdr_texture->set_data(internal, hdr_image->width, hdr_image->height, f, type, hdr_image->data);

    environment.hdr_texture = hdr_texture;

    set_active_environment(environment_entity); // TODO Paul: Transformation?

    return environment_entity;
}

void scene::set_active_environment(entity e)
{
    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");
    if (e == invalid_entity)
    {
        m_active.environment = e;
        rs->set_environment_texture(nullptr);
        return;
    }
    auto next_comp = m_environments.get_component_for_entity(e);
    if (!next_comp)
        return;

    m_active.environment = e;
    rs->set_environment_texture(next_comp->hdr_texture);
}

void scene::update(float dt)
{
    PROFILE_ZONE;
    MANGO_UNUSED(dt);
    transformation_update.execute(dt, m_transformations);
    scene_graph_update.execute(dt, m_nodes, m_transformations);
    camera_update.execute(dt, m_cameras, m_transformations);
}

void scene::render()
{
    PROFILE_ZONE;
    shared_ptr<render_system_impl> rs = m_shared_context->get_render_system_internal().lock();
    MANGO_ASSERT(rs, "Render System is expired!");

    light_submission.setup(rs);
    light_submission.execute(0.0f, m_lights);
    render_mesh.setup(rs);
    render_mesh.execute(0.0f, m_meshes, m_transformations);
}

void scene::attach(entity child, entity parent)
{
    PROFILE_ZONE;
    MANGO_ASSERT(invalid_entity != child, "Child is invalid!");
    MANGO_ASSERT(invalid_entity != parent, "Parent is invalid!");

    node_component* parent_node = m_nodes.get_component_for_entity(parent);
    if (nullptr == parent_node)
    {
        m_nodes.create_component_for(parent);
        parent_node = m_nodes.get_component_for_entity(parent);
    }
    node_component* child_node = m_nodes.get_component_for_entity(child);
    if (nullptr == child_node)
    {
        m_nodes.create_component_for(child);
        child_node = m_nodes.get_component_for_entity(child);
    }

    child_node->parent_entity = parent;

    auto sibling_entity = parent_node->child_entities;
    if (invalid_entity == sibling_entity)
        parent_node->child_entities = child;
    else
    {
        node_component* sibling_node = nullptr;
        for (int32 i = 0; i < parent_node->children_count; ++i)
        {
            sibling_node   = m_nodes.get_component_for_entity(sibling_entity);
            sibling_entity = sibling_node->next_sibling;
        }
        sibling_node->next_sibling   = child;
        child_node->previous_sibling = sibling_entity;
    }
    parent_node->children_count++;

    // reorder subtrees if necessary
    if (m_nodes.size() > 1)
    {
        NAMED_PROFILE_ZONE("Reordering on entity attachment");
        m_nodes.for_each(
            [this](node_component, int32& index) {
                entity possible_parent = m_nodes.entity_at(index);
                for (int32 j = 0; j < index; ++j)
                {
                    const node_component& possible_child = m_nodes.component_at(j);

                    if (possible_child.parent_entity == possible_parent)
                    {
                        m_nodes.move(index, j);
                        ++index;
                        break;
                    }
                }
            },
            true);
    }

    transform_component* parent_transform = m_transformations.get_component_for_entity(parent);
    if (nullptr == parent_transform)
        m_transformations.create_component_for(parent); // create transform component for parent if non-existent

    transform_component* child_transform = m_transformations.get_component_for_entity(child);
    if (nullptr == child_transform)
        m_transformations.create_component_for(child); // create transform component for child if non-existent
}

void scene::detach(entity node)
{
    PROFILE_ZONE;
    node_component* child_node = m_nodes.get_component_for_entity(node);

    if (nullptr == child_node || child_node->parent_entity == invalid_entity)
    {
        MANGO_LOG_DEBUG("Entity has no parent!");
        return;
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(node);

    if (nullptr != child_transform)
    {
        // Add transformation from parent before removing the parent
        child_transform->local_transformation_matrix = child_transform->world_transformation_matrix;
    }

    node_component* parent_node = m_nodes.get_component_for_entity(child_node->parent_entity);
    MANGO_ASSERT(parent_node, "Node with parent has no parent. ... This should not happen!");

    auto sibling_entity = parent_node->child_entities;
    if (sibling_entity == node)
    {
        parent_node->child_entities = invalid_entity;
        if (child_node->next_sibling != invalid_entity)
        {
            auto next_sibling              = m_nodes.get_component_for_entity(child_node->next_sibling);
            next_sibling->previous_sibling = invalid_entity;
            parent_node->child_entities    = child_node->next_sibling;
        }
    }
    else
    {
        node_component* next_sibling = nullptr;
        if (invalid_entity != child_node->next_sibling)
            next_sibling = m_nodes.get_component_for_entity(child_node->next_sibling);
        node_component* sibling_node = nullptr;
        for (int32 i = 0; i < parent_node->children_count; ++i)
        {
            if (sibling_entity == node)
            {
                // sibling_node is still the previous one
                if (next_sibling)
                {
                    sibling_node->next_sibling     = child_node->next_sibling;
                    next_sibling->previous_sibling = child_node->previous_sibling;
                }
                else
                    sibling_node->next_sibling = invalid_entity;
                break;
            }
            sibling_node   = m_nodes.get_component_for_entity(sibling_entity);
            sibling_entity = sibling_node->next_sibling;
        }
    }
    parent_node->children_count--;

    if (child_node->children_count == 0)
        m_nodes.sort_remove_component_from(node); // Sorting necessarry?

    if (parent_node->children_count == 0 && parent_node->parent_entity == invalid_entity)
        m_nodes.sort_remove_component_from(child_node->parent_entity); // Sorting necessarry?
}

void scene::delete_node(entity node)
{
    PROFILE_ZONE;
    node_component* child_node = m_nodes.get_component_for_entity(node);

    if (nullptr == child_node || child_node->parent_entity == invalid_entity)
    {
        MANGO_LOG_DEBUG("Entity has no parent!");
        return;
    }

    transform_component* child_transform = m_transformations.get_component_for_entity(node);

    if (nullptr != child_transform)
    {
        // Add transformation from parent before removing the parent
        child_transform->local_transformation_matrix = child_transform->world_transformation_matrix;
    }

    auto children_entity = child_node->child_entities;
    for (int32 i = 0; i < child_node->children_count; ++i)
    {
        auto children_node              = m_nodes.get_component_for_entity(children_entity);
        children_entity                 = children_node->next_sibling;
        children_node->parent_entity    = invalid_entity;
        children_node->next_sibling     = invalid_entity;
        children_node->previous_sibling = invalid_entity;
    }

    node_component* parent_node = m_nodes.get_component_for_entity(child_node->parent_entity);
    MANGO_ASSERT(parent_node, "Node with parent has no parent. ... This should not happen!");

    auto sibling_entity = parent_node->child_entities;
    if (sibling_entity == node)
    {
        parent_node->child_entities = invalid_entity;
        if (child_node->next_sibling != invalid_entity)
        {
            auto next_sibling              = m_nodes.get_component_for_entity(child_node->next_sibling);
            next_sibling->previous_sibling = invalid_entity;
            parent_node->child_entities    = child_node->next_sibling;
        }
    }
    else
    {
        node_component* next_sibling = nullptr;
        if (invalid_entity != child_node->next_sibling)
            next_sibling = m_nodes.get_component_for_entity(child_node->next_sibling);
        node_component* sibling_node = nullptr;
        for (int32 i = 0; i < parent_node->children_count; ++i)
        {
            if (sibling_entity == node)
            {
                // sibling_node is still the previous one
                if (next_sibling)
                {
                    sibling_node->next_sibling     = child_node->next_sibling;
                    next_sibling->previous_sibling = child_node->previous_sibling;
                }
                else
                    sibling_node->next_sibling = invalid_entity;
                break;
            }
            sibling_node   = m_nodes.get_component_for_entity(sibling_entity);
            sibling_entity = sibling_node->next_sibling;
        }
    }
    parent_node->children_count--;

    m_nodes.sort_remove_component_from(node); // Sorting necessary?

    if (parent_node->children_count == 0 && parent_node->parent_entity == invalid_entity)
        m_nodes.sort_remove_component_from(child_node->parent_entity); // Sorting necessary?
}

entity scene::build_model_node(tinygltf::Model& m, tinygltf::Node& n, const glm::mat4& parent_world, const std::map<int, buffer_ptr>& buffer_map)
{
    PROFILE_ZONE;
    entity node                                = create_empty();
    m_tags.create_component_for(node).tag_name = n.name;
    auto& transform                            = m_transformations.create_component_for(node);
    if (n.matrix.size() == 16)
    {
        glm::mat4 input = glm::make_mat4(n.matrix.data());
        glm::vec3 s;
        glm::vec4 p;
        glm::decompose(input, transform.scale, transform.rotation, transform.position, s, p);
    }
    else
    {
        if (n.translation.size() == 3)
        {
            transform.position = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);
        }
        if (n.rotation.size() == 4)
        {
            transform.rotation = glm::quat(static_cast<float>(n.rotation[3]), static_cast<float>(n.rotation[0]), static_cast<float>(n.rotation[1]), static_cast<float>(n.rotation[2]));
        }
        if (n.scale.size() == 3)
        {
            transform.scale = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);
        }
    }

    glm::mat4 trafo = glm::translate(glm::mat4(1.0), transform.position);
    trafo           = trafo * glm::toMat4(transform.rotation);
    trafo           = glm::scale(trafo, transform.scale);

    trafo = parent_world * trafo;

    if (n.mesh > -1)
    {
        MANGO_ASSERT(n.mesh < static_cast<int32>(m.meshes.size()), "Invalid gltf mesh!");
        build_model_mesh(node, m, m.meshes.at(n.mesh), buffer_map);
        update_scene_boundaries(trafo, m, m.meshes.at(n.mesh), m_scene_boundaries.min, m_scene_boundaries.max);
    }

    if (n.camera > -1)
    {
        MANGO_ASSERT(n.camera < static_cast<int32>(m.cameras.size()), "Invalid gltf camera!");
        build_model_camera(node, m.cameras.at(n.camera));
    }

    // build child nodes.
    for (int32 i = 0; i < static_cast<int32>(n.children.size()); ++i)
    {
        MANGO_ASSERT(n.children[i] < static_cast<int32>(m.nodes.size()), "Invalid gltf node!");

        entity child = build_model_node(m, m.nodes.at(n.children.at(i)), trafo, buffer_map);
        attach(child, node);
    }

    return node;
}

void scene::build_model_mesh(entity node, tinygltf::Model& m, tinygltf::Mesh& mesh, const std::map<int, buffer_ptr>& buffer_map)
{
    PROFILE_ZONE;
    auto& component_mesh = m_meshes.create_component_for(node);

    for (int32 i = 0; i < static_cast<int32>(mesh.primitives.size()); ++i)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        primitive_component p;
        p.vertex_array_object = vertex_array::create();
        p.topology            = static_cast<primitive_topology>(primitive.mode); // cast is okay.
        p.instance_count      = 1;
        bool has_indices      = true;

        if (primitive.indices >= 0)
        {
            const tinygltf::Accessor& index_accessor = m.accessors[primitive.indices];

            p.first      = static_cast<int32>(index_accessor.byteOffset); // TODO Paul: Is int32 big enough?
            p.count      = static_cast<int32>(index_accessor.count);      // TODO Paul: Is int32 big enough?
            p.type_index = static_cast<index_type>(index_accessor.componentType);

            auto it = buffer_map.find(index_accessor.bufferView);
            if (it == buffer_map.end())
            {
                MANGO_LOG_ERROR("No buffer data for index bufferView {0}!", index_accessor.bufferView);
                continue;
            }
            p.vertex_array_object->bind_index_buffer(it->second);
        }
        else
        {
            p.first      = 0;
            p.type_index = index_type::none;
            // p.count has to be set later.
            has_indices = false;
        }

        material_component mat;
        mat.component_material             = std::make_shared<material>();
        mat.component_material->base_color = glm::vec4(glm::vec3(0.9f), 1.0f);
        mat.component_material->metallic   = 0.0f;
        mat.component_material->roughness  = 1.0f;

        load_material(mat, primitive, m);

        component_mesh.materials.push_back(mat);

        int32 vb_idx                = 0;
        component_mesh.has_normals  = false;
        component_mesh.has_tangents = false;

        for (auto& attrib : primitive.attributes)
        {
            const tinygltf::Accessor& accessor = m.accessors[attrib.second];
            if (accessor.sparse.isSparse)
            {
                MANGO_LOG_ERROR("Models with sparse accessors are currently not supported! Undefined behavior!");
                return;
            }

            format attribute_format = get_attribute_format(static_cast<format>(accessor.componentType), accessor.type % 32); // TODO Paul: Type is not handled properly here.

            int attrib_array = -1;
            if (attrib.first.compare("POSITION") == 0)
                attrib_array = 0;
            if (attrib.first.compare("NORMAL") == 0)
            {
                component_mesh.has_normals = true;
                attrib_array               = 1;
            }
            if (attrib.first.compare("TEXCOORD_0") == 0)
                attrib_array = 2;
            if (attrib.first.compare("TANGENT") == 0)
            {
                component_mesh.has_tangents = true;
                attrib_array                = 3;
            }
            if (attrib_array > -1)
            {
                auto it = buffer_map.find(accessor.bufferView);
                if (it == buffer_map.end())
                {
                    MANGO_LOG_ERROR("No buffer data for bufferView {0}!", accessor.bufferView);
                    continue;
                }
                int32 stride = accessor.ByteStride(m.bufferViews[accessor.bufferView]);
                MANGO_ASSERT(stride > 0, "Broken gltf model! Attribute stride is {0}!", stride);
                p.vertex_array_object->bind_vertex_buffer(vb_idx, it->second, accessor.byteOffset, stride);
                p.vertex_array_object->set_vertex_attribute(attrib_array, vb_idx, attribute_format, 0);

                if (attrib_array == 0 && !has_indices)
                {
                    p.count = static_cast<int32>(accessor.count); // TODO Paul: Is int32 big enough?
                }

                vb_idx++;
            }
            else
            {
                MANGO_LOG_DEBUG("Vertex attribute array is ignored: {0}!", attrib.first);
            }
        }

        component_mesh.primitives.push_back(p);
    }
}

void scene::build_model_camera(entity node, tinygltf::Camera& camera)
{
    PROFILE_ZONE;
    auto& component_camera    = m_cameras.create_component_for(node);
    component_camera.cam_type = camera.type == "perspective" ? camera_type::perspective_camera : camera_type::orthographic_camera;
    if (component_camera.cam_type == camera_type::perspective_camera)
    {
        component_camera.z_near                             = static_cast<float>(camera.perspective.znear);
        component_camera.z_far                              = camera.perspective.zfar > 0.0 ? static_cast<float>(camera.perspective.zfar) : 10000.0f; // Infinite?
        component_camera.perspective.vertical_field_of_view = static_cast<float>(camera.perspective.yfov);
        component_camera.perspective.aspect                 = camera.perspective.aspectRatio > 0.0 ? static_cast<float>(camera.perspective.aspectRatio) : 16.0f / 9.0f;
    }
    else // orthographic
    {
        component_camera.z_near             = static_cast<float>(camera.orthographic.znear);
        component_camera.z_far              = camera.perspective.zfar > 0.0 ? static_cast<float>(camera.perspective.zfar) : 10000.0f; // Infinite?
        component_camera.orthographic.x_mag = static_cast<float>(camera.orthographic.xmag);
        component_camera.orthographic.y_mag = static_cast<float>(camera.orthographic.ymag);
    }
}

void scene::load_material(material_component& material, const tinygltf::Primitive& primitive, tinygltf::Model& m)
{
    PROFILE_ZONE;
    if (primitive.material < 0)
        return;

    const tinygltf::Material& p_m = m.materials[primitive.material];
    if (!p_m.name.empty())
    {
        MANGO_LOG_DEBUG("Loading material: {0}", p_m.name.c_str());
    }

    material.material_name                    = p_m.name;
    material.component_material->double_sided = p_m.doubleSided;

    material.component_material->use_base_color_texture         = false;
    material.component_material->use_roughness_metallic_texture = false;
    material.component_material->use_occlusion_texture          = false;
    material.component_material->use_normal_texture             = false;
    material.component_material->use_emissive_color_texture     = false;
    material.component_material->use_packed_occlusion           = false;

    auto& pbr = p_m.pbrMetallicRoughness;

    // TODO Paul: Better structure?!

    texture_configuration config;
    config.m_generate_mipmaps        = 1;
    config.m_is_standard_color_space = true;
    config.m_texture_min_filter      = texture_parameter::filter_linear_mipmap_linear;
    config.m_texture_mag_filter      = texture_parameter::filter_linear;
    config.m_texture_wrap_s          = texture_parameter::wrap_repeat;
    config.m_texture_wrap_t          = texture_parameter::wrap_repeat;

    if (pbr.baseColorTexture.index < 0)
    {
        auto col                                = pbr.baseColorFactor;
        material.component_material->base_color = glm::vec4((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
    }
    else
    {
        material.component_material->use_base_color_texture = true;
        // base color
        const tinygltf::Texture& base_col = m.textures.at(pbr.baseColorTexture.index);

        if (base_col.source < 0)
            return;

        const tinygltf::Image& image = m.images[static_cast<g_enum>(base_col.source)];

        if (base_col.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[static_cast<g_enum>(base_col.sampler)];
            config.m_texture_min_filter      = filter_parameter_from_gl(static_cast<g_enum>(sampler.minFilter));
            config.m_texture_mag_filter      = filter_parameter_from_gl(static_cast<g_enum>(sampler.magFilter));
            config.m_texture_wrap_s          = wrap_parameter_from_gl(static_cast<g_enum>(sampler.wrapS));
            config.m_texture_wrap_t          = wrap_parameter_from_gl(static_cast<g_enum>(sampler.wrapT));
        }

        config.m_is_standard_color_space = true;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr base_color           = texture::create(config);

        format f        = format::rgba;
        format internal = format::srgb8_alpha8;

        if (image.component == 1)
        {
            f = format::red;
        }
        else if (image.component == 2)
        {
            f = format::rg;
        }
        else if (image.component == 3)
        {
            f        = format::rgb;
            internal = format::srgb8;
        }

        format type = format::t_unsigned_byte;
        if (image.bits == 16)
        {
            type = format::t_unsigned_short;
        }
        else if (image.bits == 32)
        {
            type = format::t_unsigned_int;
        }

        base_color->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->base_color_texture = base_color;
    }

    // metallic / roughness
    if (pbr.metallicRoughnessTexture.index < 0)
    {
        material.component_material->metallic  = static_cast<float>(pbr.metallicFactor);
        material.component_material->roughness = static_cast<float>(pbr.roughnessFactor);
    }
    else
    {
        material.component_material->use_roughness_metallic_texture = true;
        const tinygltf::Texture& o_r_m_t                            = m.textures.at(pbr.metallicRoughnessTexture.index);

        if (o_r_m_t.source < 0)
            return;

        const tinygltf::Image& image = m.images[o_r_m_t.source];

        if (o_r_m_t.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[o_r_m_t.sampler];
            config.m_texture_min_filter      = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter      = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s          = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t          = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = false;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr o_r_m                = texture::create(config);

        format f        = format::rgba;
        format internal = format::rgba8;

        if (image.component == 1)
        {
            f = format::red;
        }
        else if (image.component == 2)
        {
            f = format::rg;
        }
        else if (image.component == 3)
        {
            f        = format::rgb;
            internal = format::rgb8;
        }

        format type = format::t_unsigned_byte;
        if (image.bits == 16)
        {
            type = format::t_unsigned_short;
        }
        else if (image.bits == 32)
        {
            type = format::t_unsigned_int;
        }

        o_r_m->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->roughness_metallic_texture = o_r_m;
    }

    // occlusion
    if (p_m.occlusionTexture.index >= 0)
    {
        if (pbr.metallicRoughnessTexture.index == p_m.occlusionTexture.index)
        {
            // occlusion packed into r channel of the roughness and metallic texture.
            material.component_material->use_packed_occlusion = true;
            material.component_material->packed_occlusion     = true;
        }
        else
        {
            material.component_material->use_occlusion_texture = true;
            material.component_material->packed_occlusion      = false;
            const tinygltf::Texture& occ                       = m.textures.at(p_m.occlusionTexture.index);
            if (occ.source < 0)
                return;

            const tinygltf::Image& image = m.images[occ.source];

            if (occ.sampler >= 0)
            {
                const tinygltf::Sampler& sampler = m.samplers[occ.sampler];
                config.m_texture_min_filter      = filter_parameter_from_gl(sampler.minFilter);
                config.m_texture_mag_filter      = filter_parameter_from_gl(sampler.magFilter);
                config.m_texture_wrap_s          = wrap_parameter_from_gl(sampler.wrapS);
                config.m_texture_wrap_t          = wrap_parameter_from_gl(sampler.wrapT);
            }

            config.m_is_standard_color_space = false;
            config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
            texture_ptr occlusion            = texture::create(config);

            format f        = format::rgba;
            format internal = format::rgba8;

            if (image.component == 1)
            {
                f = format::red;
            }
            else if (image.component == 2)
            {
                f = format::rg;
            }
            else if (image.component == 3)
            {
                f        = format::rgb;
                internal = format::rgb8;
            }

            format type = format::t_unsigned_byte;
            if (image.bits == 16)
            {
                type = format::t_unsigned_short;
            }
            else if (image.bits == 32)
            {
                type = format::t_unsigned_int;
            }

            occlusion->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
            material.component_material->occlusion_texture = occlusion;
        }
    }

    // normal
    if (p_m.normalTexture.index >= 0)
    {
        material.component_material->use_normal_texture = true;
        const tinygltf::Texture& norm                   = m.textures.at(p_m.normalTexture.index);

        if (norm.source < 0)
            return;

        const tinygltf::Image& image = m.images[norm.source];

        if (norm.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[norm.sampler];
            config.m_texture_min_filter      = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter      = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s          = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t          = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = false;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr normal_t             = texture::create(config);

        format f        = format::rgba;
        format internal = format::rgba8;

        if (image.component == 1)
        {
            f = format::red;
        }
        else if (image.component == 2)
        {
            f = format::rg;
        }
        else if (image.component == 3)
        {
            f        = format::rgb;
            internal = format::rgb8;
        }

        format type = format::t_unsigned_byte;
        if (image.bits == 16)
        {
            type = format::t_unsigned_short;
        }
        else if (image.bits == 32)
        {
            type = format::t_unsigned_int;
        }

        normal_t->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->normal_texture = normal_t;
    }

    // emissive
    if (p_m.emissiveTexture.index < 0)
    {
        auto col                                    = p_m.emissiveFactor;
        material.component_material->emissive_color = glm::vec3((float)col[0], (float)col[1], (float)col[2]);
    }
    else
    {
        material.component_material->use_emissive_color_texture = true;
        const tinygltf::Texture& emissive                       = m.textures.at(p_m.emissiveTexture.index);

        if (emissive.source < 0)
            return;

        const tinygltf::Image& image = m.images[emissive.source];

        if (emissive.sampler >= 0)
        {
            const tinygltf::Sampler& sampler = m.samplers[emissive.sampler];
            config.m_texture_min_filter      = filter_parameter_from_gl(sampler.minFilter);
            config.m_texture_mag_filter      = filter_parameter_from_gl(sampler.magFilter);
            config.m_texture_wrap_s          = wrap_parameter_from_gl(sampler.wrapS);
            config.m_texture_wrap_t          = wrap_parameter_from_gl(sampler.wrapT);
        }

        config.m_is_standard_color_space = true;
        config.m_generate_mipmaps        = calculate_mip_count(image.width, image.height);
        texture_ptr emissive_color       = texture::create(config);

        format f        = format::rgba;
        format internal = format::srgb8_alpha8;

        if (image.component == 1)
        {
            f = format::red;
        }
        else if (image.component == 2)
        {
            f = format::rg;
        }
        else if (image.component == 3)
        {
            f        = format::rgb;
            internal = format::srgb8;
        }

        format type = format::t_unsigned_byte;
        if (image.bits == 16)
        {
            type = format::t_unsigned_short;
        }
        else if (image.bits == 32)
        {
            type = format::t_unsigned_int;
        }

        emissive_color->set_data(internal, image.width, image.height, f, type, &image.image.at(0));
        material.component_material->emissive_color_texture = emissive_color;
    }

    // transparency
    if (p_m.alphaMode.compare("OPAQUE") == 0)
    {
        material.component_material->alpha_rendering = alpha_mode::mode_opaque;
        material.component_material->alpha_cutoff    = 1.0f;
    }
    if (p_m.alphaMode.compare("MASK") == 0)
    {
        material.component_material->alpha_rendering = alpha_mode::mode_mask;
        material.component_material->alpha_cutoff    = static_cast<float>(p_m.alphaCutoff);
    }
    if (p_m.alphaMode.compare("BLEND") == 0)
    {
        material.component_material->alpha_rendering = alpha_mode::mode_blend;
        material.component_material->alpha_cutoff    = 1.0f;
    }
}

static void update_scene_boundaries(glm::mat4& trafo, tinygltf::Model& m, tinygltf::Mesh& mesh, glm::vec3& min, glm::vec3& max)
{
    PROFILE_ZONE;
    if (mesh.primitives.empty())
        return;

    glm::vec3 min_a;
    glm::vec3 max_a;
    glm::vec3 center;
    glm::vec3 to_center;
    float radius;

    for (int32 j = 0; j < static_cast<int32>(mesh.primitives.size()); ++j)
    {
        const tinygltf::Primitive& primitive = mesh.primitives[j];

        auto attrib = primitive.attributes.find("POSITION");
        if (attrib == primitive.attributes.end())
            continue;

        const tinygltf::Accessor& accessor = m.accessors[attrib->second];

        max_a.x = (float)accessor.maxValues[0];
        max_a.y = (float)accessor.maxValues[1];
        max_a.z = (float)accessor.maxValues[2];
        min_a.x = (float)accessor.minValues[0];
        min_a.y = (float)accessor.minValues[1];
        min_a.z = (float)accessor.minValues[2];

        max_a = glm::vec3(trafo * glm::vec4(max_a, 1.0f));
        min_a = glm::vec3(trafo * glm::vec4(min_a, 1.0f));

        center    = (max_a + min_a) * 0.5f;
        to_center = max_a - center;
        radius    = glm::length(to_center);

        max_a = center + glm::vec3(radius);
        min_a = center - glm::vec3(radius);

        max = glm::max(max, max_a);
        min = glm::min(min, min_a);
    }
}
